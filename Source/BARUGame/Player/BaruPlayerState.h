#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "BaruPlayerState.generated.h"

class UAbilitySystemComponent;
class UBaruAttributeSet;

// UI Binding Delegate
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruReadyStatusChanged, bool, bIsReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruDBNOStatusChanged, bool, bIsDBNO);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruSanityChanged, float, NewSanity);

/**
 * 플레이어 상태 및 ASC, AttributeSet 소유
 */
UCLASS()
class BARUGAME_API ABaruPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	ABaruPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UBaruAttributeSet* GetAttributeSet() const { return AttributeSet; }
	
	// Getter & Setter
	// Todo : GAS 시스템 완전 구축시 상태를 델리게이트로 중계하는 역할만 수행하도록 제한해야 함
	UFUNCTION(BlueprintPure, Category = "BARU|PlayerState")
	bool IsReady() const { return bIsReady; }

	UFUNCTION(BlueprintPure, Category = "BARU|PlayerState")
	bool IsDBNO() const { return bIsDBNO; }

	UFUNCTION(BlueprintPure, Category = "BARU|PlayerState")
	float GetSanity() const { return Sanity; }
	
	// Server RPCs
	UFUNCTION(Server, Reliable, WithValidation, Category = "BARU|PlayerState")
	void Server_SetReadyStatus(bool bNewReady);

	UFUNCTION(Server, Reliable, WithValidation, Category = "BARU|PlayerState")
	void Server_UpdateNickname(const FString& NewNickname);
	
	UFUNCTION(BlueprintAuthorityOnly, Category = "BARU|PlayerState")
	void SetDBNOState(bool bNewDBNO);

	UFUNCTION(BlueprintAuthorityOnly, Category = "BARU|PlayerState")
	void SetSanityValue(float NewSanity);
	
public:
	// UI 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "BARU|PlayerState|Event")
	FOnBaruReadyStatusChanged OnReadyStatusChanged;

	UPROPERTY(BlueprintAssignable, Category = "BARU|PlayerState|Event")
	FOnBaruDBNOStatusChanged OnDBNOStatusChanged;

	UPROPERTY(BlueprintAssignable, Category = "BARU|PlayerState|Event")
	FOnBaruSanityChanged OnSanityChanged;
	
protected:
	// GAS Components 부착
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|GAS")
	TObjectPtr<UBaruAttributeSet> AttributeSet;
	
	// Replicated Properties & RepNotifies
	UPROPERTY(ReplicatedUsing = OnRep_IsReady, VisibleInstanceOnly, Category = "BARU|State")
	bool bIsReady = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsDBNO, VisibleInstanceOnly, Category = "BARU|State")
	bool bIsDBNO = false;

	UPROPERTY(ReplicatedUsing = OnRep_Sanity, VisibleInstanceOnly, Category = "BARU|State")
	float Sanity = 100.0f;

	UFUNCTION()
	virtual void OnRep_IsReady();

	UFUNCTION()
	virtual void OnRep_IsDBNO();

	UFUNCTION()
	virtual void OnRep_Sanity();
};
