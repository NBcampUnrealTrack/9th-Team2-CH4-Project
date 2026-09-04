#include "AbilitySystem/Abilities/BaruGameplayAbility.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Character/BaruCharacter.h"
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

void UBaruGameplayAbility::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
	
	if (ActivationPolicy == EBaruAbilityActivationPolicy::WhileInputActive)
	{
		// 키를 떼는 즉시 종료
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

APawn* UBaruGameplayAbility::GetAvatarPawnChecked() const
{
	return Cast<APawn>(GetAvatarActorFromActorInfo());
}

AController* UBaruGameplayAbility::GetControllerFromActorInfo() const
{
	if (const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo())
	{
		if (AController* PC = Info->PlayerController.Get())
		{
			return PC;
		}

		if (APawn* Pawn = Cast<APawn>(Info->OwnerActor.Get()))
		{
			return Pawn->GetController();
		}
	}
	return nullptr;
}

ABaruCharacter* UBaruGameplayAbility::GetBaruCharacterFromActorInfo() const
{
	return Cast<ABaruCharacter>(GetAvatarActorFromActorInfo());
}

UBaruAbilitySystemComponent* UBaruGameplayAbility::GetBaruAbilitySystemComponentFromActorInfo() const
{
	return Cast<UBaruAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
}