#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "CombatInterface.generated.h"

class UAbilitySystemComponent;

UINTERFACE(MinimalAPI, BlueprintType)
class UCombatInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 캐릭터와 몬스터의 전투, 피격, 사망, 그로기 및 스탯 조회를 위한 공용 인터페이스
 */

class BARUGAME_API ICombatInterface
{
	GENERATED_BODY()

public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const = 0;
	
	// ==============================================================================
	// 상태 조회 (몬스터 AI / UI 타겟팅 쿼리)
	// ==============================================================================
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "BARU|Combat")
	bool IsDead() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "BARU|Combat")
	bool IsDBNO() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "BARU|Combat")
	bool IsGroggy() const;

	// 현재/최대 체력 조회
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "BARU|Combat")
	float GetCurrentHealth() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "BARU|Combat")
	float GetMaxHealth() const;

	// 제압도(그로기 게이지) 비율 조회 (0.0 ~ 1.0)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "BARU|Combat")
	float GetSuppressionRatio() const;
	
	// ==============================================================================
	// 전투 액션 및 판정
	// ==============================================================================
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "BARU|Combat")
	void Die(AActor* Killer);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "BARU|Combat")
	void ApplyCombatDamage(float DamageAmount, const FHitResult& HitResult, AActor* DamageCauser, AController* InstigatedBy);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "BARU|Combat")
	void BreakBodyPart(FName BoneName, float Damage);
};
