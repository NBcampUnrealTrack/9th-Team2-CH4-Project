#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/BaruGameplayAbility.h"
#include "BaruGA_FireHitscan.generated.h"

class UGameplayEffect;

UCLASS(Abstract)
class BARUGAME_API UBaruGA_FireHitscan : public UBaruGameplayAbility
{
	GENERATED_BODY()

public:
	UBaruGA_FireHitscan();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	virtual void PerformFire();

protected:
	// 데미지 적용용 GE (기본값)
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Combat")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "BARU|Combat")
	float BaseDamage = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "BARU|Combat")
	float MaxRange = 10000.0f;

	// 연사 주기 (초 단위)
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Combat")
	float FireInterval = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "BARU|Combat")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_GameTraceChannel2;

	UPROPERTY(EditDefaultsOnly, Category = "BARU|Cosmetics")
	FGameplayTag FireCueTag;

private:
	FTimerHandle FireTimerHandle;
};