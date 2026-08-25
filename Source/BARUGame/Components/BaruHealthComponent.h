#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaruHealthComponent.generated.h"

class AActor;

// 액터 로컬에서 연출(애니메이션, 파티클 등)을 처리하기 위한 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FBaruOnHealthChangedSignature, class UBaruHealthComponent*, HealthComp, float, OldHealth, float, NewHealth, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBaruOnDeathSignature, AActor*, Killer);

// UI 및 외부 시스템 디커플링을 위한 로컬 메시지 구조체 (UMG 직접 참조 X)
USTRUCT(BlueprintType)
struct BARUGAME_API FBaruHealthChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> OwnerActor = nullptr;

	UPROPERTY(BlueprintReadOnly)
	float CurrentHealth = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float MaxHealth = 0.f;
};

UCLASS( ClassGroup=(Baru), meta=(BlueprintSpawnableComponent) )
class BARUGAME_API UBaruHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBaruHealthComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

public:
	// ---------------------------------------------------------
	// [서버 전용] 체력 변경 로직
	// ---------------------------------------------------------
	
	// 데미지 적용 (서버에서만 호출 유효) 
	UFUNCTION(BlueprintCallable, Category = "Baru|Health", BlueprintAuthorityOnly)
	void ApplyDamage(float DamageAmount, AActor* Instigator);

	// 회복 적용 (서버에서만 호출 유효) 
	UFUNCTION(BlueprintCallable, Category = "Baru|Health", BlueprintAuthorityOnly)
	void ApplyHeal(float HealAmount, AActor* Instigator);


	// ---------------------------------------------------------
	// Getter
	// ---------------------------------------------------------
	
	UFUNCTION(BlueprintPure, Category = "Baru|Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Baru|Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Baru|Health")
	bool IsDead() const { return bIsDead; }


	// ---------------------------------------------------------
	// 로컬 이벤트 (연출용)
	// ---------------------------------------------------------
	
	UPROPERTY(BlueprintAssignable, Category = "Baru|Health")
	FBaruOnHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Baru|Health")
	FBaruOnDeathSignature OnDeath;

protected:
	// ---------------------------------------------------------
	// 프로퍼티 및 동기화 (Data-Driven & Replicated)
	// ---------------------------------------------------------
	
	// 최대 체력 (UPrimaryDataAsset 등을 통해 초기화 권장) 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Baru|Health")
	float MaxHealth;

	// 현재 체력 (서버에서만 변경, 클라이언트는 OnRep 대기) 
	UPROPERTY(Transient, BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Baru|Health")
	float Health;

	// 사망 상태 
	UPROPERTY(Transient, BlueprintReadOnly, ReplicatedUsing = OnRep_IsDead, Category = "Baru|Health")
	bool bIsDead;

	UFUNCTION()
	virtual void OnRep_Health(float OldHealth);

	UFUNCTION()
	virtual void OnRep_IsDead();
	

private:
	// 체력 변경 시 UI 및 시스템에 전달할 브로드캐스트 공통 처리 
	void BroadcastHealthChanged(float OldHealth, float NewHealth, AActor* Instigator);
};