#include "AbilitySystem/Abilities/Weapons/BaruGA_FireHitscan.h"
#include "AbilitySystem/BaruAbilitySystemComponent.h"
#include "Character/BaruCharacter.h"
#include "Interfaces/CombatInterface.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "CollisionQueryParams.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "BaruLog.h"
#include "GameplayTags/BaruGameplayTags.h"
#include "Gameplay/Equipment/BaruEquipmentComponent.h"
#include "Gameplay/Weapon/BaruWeaponBase.h"

UBaruGA_FireHitscan::UBaruGA_FireHitscan()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    // 사격 불가 State Tag (장전중, 사망, DBNO)
    ActivationBlockedTags.AddTag(FBaruGameplayTags::Get().State_Combat_Reloading);
    ActivationBlockedTags.AddTag(FBaruGameplayTags::Get().State_Dead);
    ActivationBlockedTags.AddTag(FBaruGameplayTags::Get().State_DBNO);
}

void UBaruGA_FireHitscan::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 첫 발 격발
    PerformFire();

    // 연사 모드일 경우 타이머 가동 (연사 모드 : WhileInputActive)
    if (ActivationPolicy == EBaruAbilityActivationPolicy::WhileInputActive && FireInterval > 0.0f)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                FireTimerHandle,
                this,
                &UBaruGA_FireHitscan::PerformFire,
                FireInterval,
                true
            );
        }
    }
    else
    {
        // 단발 모드는 1발 발사 후 종료 (단발 모드 : OnInputTriggered)
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }
}

void UBaruGA_FireHitscan::PerformFire()
{
    APawn* AvatarPawn = GetAvatarPawnChecked();
    AController* Controller = GetControllerFromActorInfo();
    if (!AvatarPawn || !Controller)
    {
        return;
    }
    
    // 현재 들고 있는 무기 및 잔여 탄약 검사
    ABaruCharacter* BaruChar = GetBaruCharacterFromActorInfo();
    UBaruEquipmentComponent* EquipComp = BaruChar ? BaruChar->FindComponentByClass<UBaruEquipmentComponent>() : nullptr;
    ABaruWeaponBase* ActiveWeapon = EquipComp ? EquipComp->GetActiveWeapon() : nullptr;

    if (!ActiveWeapon || ActiveWeapon->GetCurrentAmmo() <= 0)
    {
        // 탄약이 없으면 자동 재장전 트리거 후 사격 중단
        if (UBaruAbilitySystemComponent* SourceASC = GetBaruAbilitySystemComponentFromActorInfo())
        {
            SourceASC->TryActivateAbilityByTag(FBaruGameplayTags::Get().InputTag_Reload);
        }
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }

    // 탄약/쿨다운 재검사
    if (!CommitCheck(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }
    
    // 서버에서 탄약 1발 소모
    if (HasAuthority(&CurrentActivationInfo))
    {
        ActiveWeapon->ConsumeAmmo(1);
    }

    FVector ViewLoc;
    FRotator ViewRot;
    Controller->GetPlayerViewPoint(ViewLoc, ViewRot);

    // 탄 퍼짐 연산
    const float SpreadHalfAngleRad = FMath::DegreesToRadians(SpreadAngle * 0.5f);
    const FVector FireDir = FMath::VRandCone(ViewRot.Vector(), SpreadHalfAngleRad);
    const FVector TraceEnd = ViewLoc + (FireDir * MaxRange);

    // 로컬 화면 반동(FBaruRecoilData 만들어지면 주석 해제)
    // if (ABaruCharacter* BaruChar = GetBaruCharacterFromActorInfo())
    // {
    //     if (BaruChar->IsLocallyControlled())
    //     {
    //         BaruChar->ApplyRecoil(RecoilData);
    //     }
    // }
    
    // 라인트레이스 및 충돌 연산
    FCollisionQueryParams Params(TEXT("FireHitscanTrace"), true, AvatarPawn);
    Params.bReturnPhysicalMaterial = true;

    FHitResult HitResult;
    const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, ViewLoc, TraceEnd, TraceChannel, Params);

    UBaruAbilitySystemComponent* SourceASC = GetBaruAbilitySystemComponentFromActorInfo();

    // GameplayCue : Fire 연출
    if (SourceASC && FireCueTag.IsValid())
    {
        FGameplayCueParameters FireParams;
        FireParams.Location = ViewLoc;
        FireParams.Normal = ViewRot.Vector();
        FireParams.EffectCauser = GetAvatarActorFromActorInfo();
        SourceASC->ExecuteGameplayCue(FireCueTag, FireParams);
    }
    
    // GameplayCue : Hit 연출
    if (SourceASC && ImpactCueTag.IsValid() && bHit)
    {
        FGameplayCueParameters ImpactParams;
        ImpactParams.Location = HitResult.ImpactPoint;
        ImpactParams.Normal = HitResult.ImpactNormal;
        ImpactParams.PhysicalMaterial = HitResult.PhysMaterial;
        SourceASC->ExecuteGameplayCue(ImpactCueTag, ImpactParams);
    }

    // 피격 판정 및 데미지 (Server)
    if (HasAuthority(&CurrentActivationInfo) && bHit && HitResult.GetActor())
    {
        AActor* HitActor = HitResult.GetActor();
        if (HitActor->Implements<UCombatInterface>())
        {
            if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(HitActor))
            {
                if (UAbilitySystemComponent* TargetASC = CombatInterface->GetAbilitySystemComponent())
                {
                    if (SourceASC && DamageEffectClass)
                    {
                        SourceASC->ApplyDamageEffectToTarget(DamageEffectClass, TargetASC, BaseDamage);
                    }
                }
            }
        }
    }
    
    // 이번 격발로 마지막 발(30번째 또는 6번째)을 소진했을 경우 자동 재장전 트리거
    if (ActiveWeapon->GetCurrentAmmo() <= 0)
    {
        if (SourceASC)
        {
            SourceASC->TryActivateAbilityByTag(FBaruGameplayTags::Get().InputTag_Reload);
        }
    }
}

void UBaruGA_FireHitscan::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FireTimerHandle);
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}