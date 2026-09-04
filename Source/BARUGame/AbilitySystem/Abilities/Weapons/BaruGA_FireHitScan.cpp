#include "AbilitySystem/Abilities/Weapons/BaruGA_FireHitscan.h"
#include "AbilitySystem/BaruAbilitySystemComponent.h"
#include "Interfaces/CombatInterface.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "CollisionQueryParams.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "BaruLog.h"

UBaruGA_FireHitscan::UBaruGA_FireHitscan()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    // 사격 불가 State Tag (장전중, 사망, DBNO)
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Combat.Reloading")));
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Dead")));
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.DBNO")));
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

    // 탄약/쿨다운 재검사
    if (!CommitCheck(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }

    FVector ViewLoc;
    FRotator ViewRot;
    Controller->GetPlayerViewPoint(ViewLoc, ViewRot);

    const FVector TraceEnd = ViewLoc + (ViewRot.Vector() * MaxRange);

    FCollisionQueryParams Params(TEXT("FireHitscanTrace"), true, AvatarPawn);
    Params.bReturnPhysicalMaterial = true;

    FHitResult HitResult;
    const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, ViewLoc, TraceEnd, TraceChannel, Params);

    UBaruAbilitySystemComponent* SourceASC = GetBaruAbilitySystemComponentFromActorInfo();

    // GameplayCue : Fire 연출
    if (SourceASC && FireCueTag.IsValid())
    {
        FGameplayCueParameters CueParams;
        CueParams.Location = HitResult.bBlockingHit ? HitResult.ImpactPoint : TraceEnd;
        CueParams.Normal = HitResult.ImpactNormal;
        SourceASC->ExecuteGameplayCue(FireCueTag, CueParams);
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