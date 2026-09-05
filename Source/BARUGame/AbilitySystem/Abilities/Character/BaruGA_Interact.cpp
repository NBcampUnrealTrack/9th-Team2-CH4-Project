#include "AbilitySystem/Abilities/Character/BaruGA_Interact.h"
#include "Interfaces/InteractableInterface.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
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

    APawn* AvatarPawn = GetAvatarPawnChecked();
    AController* Controller = GetControllerFromActorInfo();

    if (!AvatarPawn || !Controller)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    FVector ViewLoc;
    FRotator ViewRot;
    Controller->GetPlayerViewPoint(ViewLoc, ViewRot);

    const FVector TraceEnd = ViewLoc + (ViewRot.Vector() * TraceDistance);

    FCollisionQueryParams Params(TEXT("InteractTrace"), true, AvatarPawn);
    FHitResult HitResult;

    const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, ViewLoc, TraceEnd, TraceChannel, Params);

    if (bHit && HitResult.GetActor())
    {
        AActor* TargetActor = HitResult.GetActor();
        if (TargetActor->Implements<UInteractableInterface>())
        {
            if (IInteractableInterface::Execute_CanInteract(TargetActor, AvatarPawn))
            {
                // 상호작용 실행은 서버 권한에서 최종 확정
                if (HasAuthority(&ActivationInfo))
                {
                    IInteractableInterface::Execute_ExecuteInteraction(TargetActor, AvatarPawn);
                    BARU_NET_LOG(AvatarPawn, LogBaruItem, Log, TEXT("Interacted with: %s"), *TargetActor->GetName());
                }
            }
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}