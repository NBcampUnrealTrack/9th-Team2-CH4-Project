#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/BaruGameplayAbility.h"
#include "BaruGA_MonsterAttack.generated.h"

class UAnimMontage;
class UGameplayEffect;

UCLASS(Abstract)
class BARUGAME_API UBaruGA_MonsterAttack : public UBaruGameplayAbility
{
	GENERATED_BODY()

public:
	UBaruGA_MonsterAttack();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 실제 타격 판정 실행 (몽타주 노티파이 또는 딜레이 후 호출)
	UFUNCTION(BlueprintCallable, Category = "BARU|Monster")
	virtual void PerformMeleeAttackTrace();

protected:
	// 기본값 세팅
	
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Monster|Attack")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "BARU|Monster|Attack")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "BARU|Monster|Attack")
	float AttackDamage = 35.0f;

	UPROPERTY(EditDefaultsOnly, Category = "BARU|Monster|Attack")
	float AttackRange = 180.0f;

	UPROPERTY(EditDefaultsOnly, Category = "BARU|Monster|Attack")
	float AttackRadius = 50.0f;
};