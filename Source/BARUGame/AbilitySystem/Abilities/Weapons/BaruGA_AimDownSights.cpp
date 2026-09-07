#include "AbilitySystem/Abilities/Weapons/BaruGA_AimDownSights.h"
#include "AbilitySystemComponent.h"
#include "Character/BaruCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameplayTags/BaruGameplayTags.h"
#include "BaruLog.h"

//Todo : BaruCharacer 또는 캐릭터 부착 컴포넌트에 FOV 제어 함수가 추가된다면 주석을 해제

UBaruGA_AimDownSights::UBaruGA_AimDownSights()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

    // 우클릭을 누르고 있는 동안만 유지
    ActivationPolicy = EBaruAbilityActivationPolicy::WhileInputActive;

    // 사상자/재장전/스프린트 중에는 조준 불가
    ActivationBlockedTags.AddTag(FBaruGameplayTags::Get().State_Dead);
    ActivationBlockedTags.AddTag(FBaruGameplayTags::Get().State_DBNO);
    ActivationBlockedTags.AddTag(FBaruGameplayTags::Get().State_Combat_Reloading);

    // 조준 중에는 State.Combat.Aiming 태그를 캐릭터에 소유
    ActivationOwnedTags.AddTag(FBaruGameplayTags::Get().State_Combat_Aiming);
}

void UBaruGA_AimDownSights::ActivateAbility(
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

    // 1. 조준 중 이동속도 감소 등 상태 GE 적용
    if (AimingEffectClass && ActorInfo->AbilitySystemComponent.IsValid())
    {
        FGameplayEffectContextHandle Context = ActorInfo->AbilitySystemComponent->MakeEffectContext();
        Context.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());

        ActiveAimingEffectHandle = ActorInfo->AbilitySystemComponent->ApplyGameplayEffectToSelf(
            AimingEffectClass.GetDefaultObject(),
            1.0f,
            Context
        );
    }

    // 2. 로컬 화면 카메라 FOV 변경 연동 (클라이언트/호스트 즉시 반영)
    // if (ABaruCharacter* Character = GetBaruCharacterFromActorInfo())
    // {
    //     if (Character->IsLocallyControlled())
    //     {
    //         Character->SetTargetCameraFOV(AimTargetFOV);
    //     }
    // }
    //
    // BARU_NET_LOG(GetAvatarActorFromActorInfo(), LogBaruCombat, Verbose, TEXT("AimDownSights Activated. TargetFOV: %.1f"), AimTargetFOV);
}

void UBaruGA_AimDownSights::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    // 1. 조준용 GE 해제
    if (ActiveAimingEffectHandle.IsValid() && ActorInfo->AbilitySystemComponent.IsValid())
    {
        ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveAimingEffectHandle);
        ActiveAimingEffectHandle.Invalidate();
    }

    // 2. 기본 시야각(90)으로 복구
    // if (ABaruCharacter* Character = GetBaruCharacterFromActorInfo())
    // {
    //     if (Character->IsLocallyControlled())
    //     {
    //         Character->ResetCameraFOV();
    //     }
    // }

    BARU_NET_LOG(GetAvatarActorFromActorInfo(), LogBaruCombat, Verbose, TEXT("AimDownSights Deactivated. Reset FOV."));

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}