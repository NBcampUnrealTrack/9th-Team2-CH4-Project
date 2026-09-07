#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/BaruGameplayAbility.h"
#include "BaruGA_ThrowGrenade.generated.h"

class AActor;
class UAnimMontage;

UCLASS(Abstract)
class BARUGAME_API UBaruGA_ThrowGrenade : public UBaruGameplayAbility
{
	GENERATED_BODY()

public:
	UBaruGA_ThrowGrenade();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Grenade")
	TSubclassOf<AActor> GrenadeProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "BARU|Grenade")
	float ThrowImpulse = 1500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "BARU|Grenade")
	TObjectPtr<UAnimMontage> ThrowMontage;
};