#include "AbilitySystem/Abilities/Weapons/BaruGA_Reload.h"
#include "AbilitySystem/BaruAbilitySystemComponent.h"
#include "GameplayTags/BaruGameplayTags.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "BaruLog.h"
#include "GameFramework/Character.h"

UBaruGA_Reload::UBaruGA_Reload()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    ActivationPolicy = EBaruAbilityActivationPolicy::OnInputTriggered;

    // 재장전 불가 State Tag (재장전, 사망, DBNO)
    ActivationBlockedTags.AddTag(FBaruGameplayTags::Get().State_Combat_Reloading);
    ActivationBlockedTags.AddTag(FBaruGameplayTags::Get().State_Dead);
    ActivationBlockedTags.AddTag(FBaruGameplayTags::Get().State_DBNO);
    
    // 태그 부여
    ActivationOwnedTags.AddTag(FBaruGameplayTags::Get().State_Combat_Reloading);
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
    // TODO: 장착 컴포넌트나 무기 액터의 탄창 수치(MagazineCapacity) 복구
    BARU_NET_LOG(GetAvatarActorFromActorInfo(), LogBaruCombat, Log, TEXT("Reload Completed."));

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