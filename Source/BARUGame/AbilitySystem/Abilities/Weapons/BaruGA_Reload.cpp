#include "AbilitySystem/Abilities/Weapons/BaruGA_Reload.h"
#include "AbilitySystem/BaruAbilitySystemComponent.h"
#include "GameplayTags/BaruGameplayTags.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "BaruLog.h"
#include "GameFramework/Character.h"
#include "Animation/AnimMontage.h"
#include "Character/BaruCharacter.h"
#include "Gameplay/Equipment/BaruEquipmentComponent.h"
#include "Gameplay/Weapon/BaruWeaponBase.h"

UBaruGA_Reload::UBaruGA_Reload()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    ActivationPolicy = EBaruAbilityActivationPolicy::OnInputTriggered;
    
    InputTag = FBaruGameplayTags::Get().InputTag_Reload;
    SetAssetTags(FGameplayTagContainer(FBaruGameplayTags::Get().Ability_Action_Reload));

    // 재장전 불가 State Tag (재장전, 사망, DBNO)
    ActivationBlockedTags.AddTag(FBaruGameplayTags::Get().State_Combat_Reloading);
    ActivationBlockedTags.AddTag(FBaruGameplayTags::Get().State_Dead);
    ActivationBlockedTags.AddTag(FBaruGameplayTags::Get().State_DBNO);
    
    // 태그 부여
    ActivationOwnedTags.AddTag(FBaruGameplayTags::Get().State_Combat_Reloading);
}

ABaruWeaponBase* UBaruGA_Reload::GetActiveWeaponFromActorInfo() const
{
    if (const ABaruCharacter* Character = GetBaruCharacterFromActorInfo())
    {
        if (const UBaruEquipmentComponent* EquipComp = Character->FindComponentByClass<UBaruEquipmentComponent>())
        {
            return EquipComp->GetActiveWeapon();
        }
    }
    return nullptr;
}

bool UBaruGA_Reload::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    OUT FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }

    ABaruWeaponBase* ActiveWeapon = Cast<ABaruWeaponBase>(GetCurrentSourceObject());
    if (!IsValid(ActiveWeapon))
    {
        ActiveWeapon = GetActiveWeaponFromActorInfo();
    }
    
    if (!IsValid(ActiveWeapon))
    {
        return false;
    }

    // 탄창이 이미 가득 차 있다면 장전 불가
    if (ActiveWeapon->IsMagazineFull())
    {
        return false;
    }

    return true;
}

void UBaruGA_Reload::ActivateAbility(
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

    float Duration = FallbackReloadDuration;

    // 몽타주 재생 (시각 연출)
    if (ReloadMontage && ActorInfo->AvatarActor.IsValid())
    {
        if (ACharacter* Char = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
        {
            const float MontageLength = Char->PlayAnimMontage(ReloadMontage);
            if (MontageLength > 0.0f)
            {
                Duration = MontageLength;
            }
        }
    }

    // 서버 권한에서만 타이머 가동 (클라이언트 레이스 컨디션 방지)
    if (ActorInfo && ActorInfo->IsNetAuthority())
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                ReloadTimerHandle,
                this,
                &UBaruGA_Reload::OnReloadCompleted,
                Duration,
                false
            );
        }
    }
}

void UBaruGA_Reload::OnReloadCompleted()
{
    if (HasAuthority(&CurrentActivationInfo))
    {
        ABaruWeaponBase* ActiveWeapon = Cast<ABaruWeaponBase>(GetCurrentSourceObject());
        if (!IsValid(ActiveWeapon))
        {
            ActiveWeapon = GetActiveWeaponFromActorInfo();
        }

        if (IsValid(ActiveWeapon))
        {
            ActiveWeapon->RestoreFullAmmo();
            BARU_NET_LOG(GetAvatarActorFromActorInfo(), LogBaruCombat, Log,
                TEXT("재장전 완료. 탄약 복구 완료: %d / %d"),
                ActiveWeapon->GetCurrentAmmo(), ActiveWeapon->GetMagazineCapacity());
        }

        // 서버 종료 복제를 통해 클라이언트 어빌리티도 함께 종료
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
}

void UBaruGA_Reload::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ReloadTimerHandle);
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}