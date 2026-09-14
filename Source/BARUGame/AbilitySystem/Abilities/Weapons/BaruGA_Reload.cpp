#include "AbilitySystem/Abilities/Weapons/BaruGA_Reload.h"
#include "AbilitySystem/BaruAbilitySystemComponent.h"
#include "GameplayTags/BaruGameplayTags.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "BaruLog.h"
#include "GameFramework/Character.h"
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

    ABaruWeaponBase* ActiveWeapon = GetActiveWeaponFromActorInfo();
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

    // 몽타주 재생
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

void UBaruGA_Reload::OnReloadCompleted()
{
    // 서버 권한에서 탄창을 최대치(라이플: 30, 리볼버: 6)로 즉시 복구
    if (HasAuthority(&CurrentActivationInfo)) // [수정]
    {
        if (ABaruWeaponBase* ActiveWeapon = GetActiveWeaponFromActorInfo())
        {
            ActiveWeapon->RestoreFullAmmo();
            BARU_NET_LOG(GetAvatarActorFromActorInfo(), LogBaruCombat, Log,
                TEXT("재장전 완료. 탄약 복구 완료: %d / %d"),
                ActiveWeapon->GetCurrentAmmo(), ActiveWeapon->GetMagazineCapacity());
        }
    }

    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
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