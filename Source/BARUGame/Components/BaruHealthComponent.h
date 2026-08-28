#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "BaruHealthComponent.generated.h"

class UAbilitySystemComponent;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FBaruOnHealthChangedSignature, class UBaruHealthComponent*, HealthComp, float, OldHealth, float, NewHealth, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBaruOnDeathSignature, AActor*, Killer);

// ★[추가] 최대 체력 전용 델리게이트.
//   기존에는 MaxHealth 가 바뀌어도 OnHealthChanged(현재체력, 현재체력) 으로만 알려줘서
//   UI 가 "최대치가 바뀌었다"는 사실을 알 방법이 없었습니다(폴링해야만 했음).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBaruOnMaxHealthChangedSignature, float, OldMaxHealth, float, NewMaxHealth);

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

	// ★[추가] UI 프로그레스바용 0~1 정규화 값. MaxHealth 가 0일 때 0으로 나누는 사고를 막습니다.
	UFUNCTION(BlueprintPure, Category = "BARU|Health")
	float GetHealthNormalized() const;

	// [수정] bIsDead 변수를 리턴하지 않고 인터페이스를 사용하도록 변경
	UFUNCTION(BlueprintPure, Category = "BARU|Health")
	bool IsDead() const; 

	// ★[추가] 사망 통지 진입점.
	//   OnDeath 델리게이트가 선언만 되고 어디서도 Broadcast 되지 않는 죽은 델리게이트였습니다.
	//   ABaruPlayerState::SetDeadState() 가 이 함수를 호출합니다.
	void NotifyDeath(AActor* Killer);

	// UI 및 이펙트 재생용 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "BARU|Health")
	FBaruOnHealthChangedSignature OnHealthChanged;

	// ★[추가]
	UPROPERTY(BlueprintAssignable, Category = "BARU|Health")
	FBaruOnMaxHealthChangedSignature OnMaxHealthChanged;

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

	// [삭제] bool bIsDead 변수 삭제 (중복 상태 관리 방지)
};