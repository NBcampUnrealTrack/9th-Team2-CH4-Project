#include "AbilitySystem/Abilities/BaruGameplayAbility.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "AbilitySystem/BaruAbilitySystemComponent.h"
#include "BaruLog.h"

UBaruGameplayAbility::UBaruGameplayAbility()
{
	// 기본 복제 인스턴싱 정책 = 액터마다 인스턴스화
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	bReplicateInputDirectly = true;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UBaruGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	if (ActivationPolicy == EBaruAbilityActivationPolicy::OnSpawn)
	{
		if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
		{
			ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
		}
	}
}

APawn* UBaruGameplayAbility::GetAvatarPawnChecked() const
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	return Cast<APawn>(Avatar);
}

AController* UBaruGameplayAbility::GetControllerFromActorInfo() const
{
	if (const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo())
	{
		if (AController* PC = Info->PlayerController.Get())
		{
			return PC;
		}

		AActor* OwnerActor = Info->OwnerActor.Get();
		if (APawn* Pawn = Cast<APawn>(OwnerActor))
		{
			return Pawn->GetController();
		}
	}
	return nullptr;
}