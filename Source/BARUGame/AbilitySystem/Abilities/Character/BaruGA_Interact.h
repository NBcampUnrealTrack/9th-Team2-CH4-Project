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

protected:
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Interaction")
	float TraceDistance = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "BARU|Interaction")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_GameTraceChannel3; // Interaction
};