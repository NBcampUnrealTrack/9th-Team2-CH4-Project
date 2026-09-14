#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/BaruGameplayAbility.h"
#include "BaruGA_Reload.generated.h"

class UAnimMontage;
class ABaruWeaponBase;

UCLASS(Abstract)
class BARUGAME_API UBaruGA_Reload : public UBaruGameplayAbility
{
	GENERATED_BODY()

public:
	UBaruGA_Reload();
	
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	
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
	UFUNCTION()
	void OnReloadCompleted();
	
	ABaruWeaponBase* GetActiveWeaponFromActorInfo() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Reload")
	TObjectPtr<UAnimMontage> ReloadMontage;

	// 몽타주가 없을 때 강제로 대기 (재장전 시간 2초)
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Reload")
	float FallbackReloadDuration = 2.0f;

private:
	FTimerHandle ReloadTimerHandle;
};