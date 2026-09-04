#include "AbilitySystem/Abilities/Weapons/BaruGA_ThrowGrenade.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"

UBaruGA_ThrowGrenade::UBaruGA_ThrowGrenade()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    ActivationPolicy = EBaruAbilityActivationPolicy::OnInputTriggered;

    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Combat.Reloading")));
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.Dead")));
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("State.DBNO")));
}

void UBaruGA_ThrowGrenade::ActivateAbility(
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

    if (HasAuthority(&CurrentActivationInfo))
    {
        APawn* AvatarPawn = GetAvatarPawnChecked();
        AController* Controller = GetControllerFromActorInfo();

        if (AvatarPawn && Controller && GrenadeProjectileClass)
        {
            FVector ViewLoc;
            FRotator ViewRot;
            Controller->GetPlayerViewPoint(ViewLoc, ViewRot);

            const FVector SpawnLoc = ViewLoc + (ViewRot.Vector() * 50.0f);
            
            FActorSpawnParameters SpawnParams;
            SpawnParams.Owner = AvatarPawn;
            SpawnParams.Instigator = AvatarPawn;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            if (AActor* SpawnedGrenade = GetWorld()->SpawnActor<AActor>(GrenadeProjectileClass, SpawnLoc, ViewRot, SpawnParams))
            {
                if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(SpawnedGrenade->GetRootComponent()))
                {
                    const FVector LaunchVelocity = (ViewRot.Vector() + FVector(0.f, 0.f, 0.2f)).GetSafeNormal() * ThrowImpulse;
                    RootPrim->AddImpulse(LaunchVelocity, NAME_None, true);
                }
            }
        }
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}