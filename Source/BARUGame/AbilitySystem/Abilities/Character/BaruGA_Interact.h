#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/BaruGameplayAbility.h"
#include "BaruGA_Interact.generated.h"

UCLASS(Abstract)
class BARUGAME_API UBaruGA_Interact : public UBaruGameplayAbility
{
	GENERATED_BODY()

public:
	UBaruGA_Interact();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	void TickInteractCheck();
	AActor* PerformTrace();
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Interaction")
	float TraceDistance = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "BARU|Interaction")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_GameTraceChannel3; // Interaction
	
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentTargetActor;

	float CurrentHoldTime = 0.0f;
	float RequiredDuration = 0.0f;

	FTimerHandle HoldTimerHandle;
};