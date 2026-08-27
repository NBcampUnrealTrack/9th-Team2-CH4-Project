#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "BaruHealthComponent.generated.h"

class UAbilitySystemComponent;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FBaruOnHealthChangedSignature, class UBaruHealthComponent*, HealthComp, float, OldHealth, float, NewHealth, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBaruOnDeathSignature, AActor*, Killer);

UCLASS( ClassGroup=(BARU), meta=(BlueprintSpawnableComponent) )
class BARUGAME_API UBaruHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public: 
	UBaruHealthComponent();

	// PlayerState의 ASC를 주입받아 초기화합니다.
	UFUNCTION(BlueprintCallable, Category = "BARU|Health")
	void InitializeWithAbilitySystem(UAbilitySystemComponent* InASC);

	UFUNCTION(BlueprintCallable, Category = "BARU|Health")
	void UninitializeFromAbilitySystem();

	// GAS AttributeSet에서 값을 직접 가져옵니다.
	UFUNCTION(BlueprintPure, Category = "BARU|Health")
	float GetHealth() const;

	UFUNCTION(BlueprintPure, Category = "BARU|Health")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "BARU|Health")
	bool IsDead() const { return bIsDead; }

	// UI 및 이펙트 재생용 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "BARU|Health")
	FBaruOnHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "BARU|Health")
	FBaruOnDeathSignature OnDeath;

protected:
	virtual void OnUnregister() override;

	// GAS 속성 변경 콜백 함수
	virtual void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	virtual void HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData);

protected:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	bool bIsDead;
};