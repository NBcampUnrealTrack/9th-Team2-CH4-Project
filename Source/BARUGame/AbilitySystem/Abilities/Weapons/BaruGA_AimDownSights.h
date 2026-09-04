#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/BaruGameplayAbility.h"
#include "BaruGA_AimDownSights.generated.h"

class UGameplayEffect;

/**
 * 무기 정조준(ADS) 및 시야각(FOV) 줌 베이스 Gameplay Ability
 */
UCLASS(Abstract)
class BARUGAME_API UBaruGA_AimDownSights : public UBaruGameplayAbility
{
	GENERATED_BODY()

public:
	UBaruGA_AimDownSights();

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
	// 정조준 시 적용할 목표 카메라 FOV (기본 90 -> 라이플: 65~70, 리볼버: 75~80)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Aim")
	float AimTargetFOV = 70.0f;

	// 조준 중 이동속도 감소 및 제어용 Infinite GE (선택 사항)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Aim")
	TSubclassOf<UGameplayEffect> AimingEffectClass;

private:
	FActiveGameplayEffectHandle ActiveAimingEffectHandle;
};