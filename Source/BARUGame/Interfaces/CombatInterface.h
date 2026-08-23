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

class BARUGAME_API ICombatInterface
{
	GENERATED_BODY()

public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const = 0;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	bool IsDead() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	bool IsDBNO() const;
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	void Die(AActor* Killer);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	void ApplyCombatDamage(float DamageAmount, const FHitResult& HitResult, AActor* DamageCauser, AController* InstigatedBy);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	void BreakBodyPart(FName BoneName, float Damage);
};
