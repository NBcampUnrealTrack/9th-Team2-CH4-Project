#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/BaruGameplayAbility.h"
#include "Animation/AnimInstance.h"
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
	
	// Ability 종료 시 Montage Notify 연결을 안전하게 해제
	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

protected:
	
	// 실제 타격 판정 실행 (몽타주 노티파이 또는 딜레이 후 호출)
	UFUNCTION(BlueprintCallable, Category = "BARU|Monster")
	virtual void PerformMeleeAttackTrace();
	
	// 공격 몽타주가 정상적으로 끝났을 때 Ability 종료
	UFUNCTION()
	void HandleAttackMontageCompleted();

	// 공격 몽타주가 취소되거나 중단됐을 때 Ability 종료
	UFUNCTION()
	void HandleAttackMontageCancelled();
	
	// 몽타주의 AttackHit Notify가 실행될 때 호출
	UFUNCTION()
	void HandleAttackMontageNotifyBegin(
		FName NotifyName,
		const FBranchingPointNotifyPayload& BranchingPointPayload
	);
	
	// 한 번의 공격에서 타격 판정이 중복 실행되는 것을 방지
	bool bAttackHitProcessed = false;
	
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