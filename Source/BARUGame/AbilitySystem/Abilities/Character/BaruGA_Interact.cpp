#include "AbilitySystem/Abilities/Character/BaruGA_Interact.h"
#include "Interfaces/InteractableInterface.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "BaruLog.h"
#include "GameplayTags/BaruGameplayTags.h"

UBaruGA_Interact::UBaruGA_Interact()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    
    ActivationPolicy = EBaruAbilityActivationPolicy::OnInputTriggered;

    ActivationBlockedTags.AddTag(FBaruGameplayTags::Get().State_Dead);
    ActivationBlockedTags.AddTag(FBaruGameplayTags::Get().State_DBNO);
}

void UBaruGA_Interact::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    CurrentHoldTime = 0.0f;
    CurrentTargetActor = PerformTrace();

    APawn* AvatarPawn = GetAvatarPawnChecked();
    if (!AvatarPawn || !CurrentTargetActor.IsValid())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 대상이 상호작용 가능한 상태인지 체크
    if (!IInteractableInterface::Execute_CanInteract(CurrentTargetActor.Get(), AvatarPawn))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 요구 홀드 시간 계산 (일반 문/아이템은 0.0초, DBNO 팀원은 3.0초)
    RequiredDuration = IInteractableInterface::Execute_GetInteractionDuration(CurrentTargetActor.Get());

    // 즉시 실행 상호작용 (문, 아이템)
    if (RequiredDuration <= 0.0f)
    {
        if (HasAuthority(&ActivationInfo))
        {
            IInteractableInterface::Execute_ExecuteInteraction(CurrentTargetActor.Get(), AvatarPawn);
        }
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    // 홀드형 상호작용 (팀원 소생 등): 0.05초 주기로 트레이스 및 시간 누적 타이머 시작
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            HoldTimerHandle,
            this,
            &UBaruGA_Interact::TickInteractCheck,
            0.05f,
            true
        );
    }
}

void UBaruGA_Interact::TickInteractCheck()
{
    APawn* AvatarPawn = GetAvatarPawnChecked();
    AActor* HitTarget = PerformTrace();

    // 대상을 도중에 놓쳤거나 대상이 바뀐 경우 즉시 취소
    if (!HitTarget || HitTarget != CurrentTargetActor.Get())
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }

    CurrentHoldTime += 0.05f;

    // 3초 완주 시 상호작용 실행
    if (CurrentHoldTime >= RequiredDuration)
    {
        if (HasAuthority(&CurrentActivationInfo))
        {
            IInteractableInterface::Execute_ExecuteInteraction(CurrentTargetActor.Get(), AvatarPawn);
            BARU_NET_LOG(AvatarPawn, LogBaruCombat, Log, TEXT("Hold Interaction Succeeded with %s"), *CurrentTargetActor->GetName());
        }

        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
}

AActor* UBaruGA_Interact::PerformTrace()
{
    APawn* AvatarPawn = GetAvatarPawnChecked();
    AController* Controller = GetControllerFromActorInfo();
    if (!AvatarPawn || !Controller) return nullptr;

    FVector ViewLoc;
    FRotator ViewRot;
    Controller->GetPlayerViewPoint(ViewLoc, ViewRot);

    const FVector TraceEnd = ViewLoc + (ViewRot.Vector() * TraceDistance);
    FCollisionQueryParams Params(TEXT("InteractTrace"), true, AvatarPawn);
    FHitResult HitResult;

    if (GetWorld()->LineTraceSingleByChannel(HitResult, ViewLoc, TraceEnd, TraceChannel, Params))
    {
        if (HitResult.GetActor() && HitResult.GetActor()->Implements<UInteractableInterface>())
        {
            return HitResult.GetActor();
        }
    }

    return nullptr;
}

void UBaruGA_Interact::InputReleased(
    const FGameplayAbilitySpecHandle Handle, 
    const FGameplayAbilityActorInfo* ActorInfo, 
    const FGameplayAbilityActivationInfo ActivationInfo)
{
    // F키를 도중에 떼면 소생 취소
    EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
    Super::InputReleased(Handle, ActorInfo, ActivationInfo);
}

void UBaruGA_Interact::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(HoldTimerHandle);
    }

    CurrentHoldTime = 0.0f;
    CurrentTargetActor.Reset();

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}