#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"

// UI Binding Delegate (generated.h 위에 선언)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruReadyStatusChanged, bool, bIsReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruDBNOStatusChanged, bool, bIsDBNO);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruSanityChanged, float, NewSanity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBaruHealthChanged, float, CurrentHealth, float, MaxHealth);

// generated.h는 항상 include 구문 및 델리게이트 선언 최하단에 위치해야 합니다.
#include "BaruPlayerState.generated.h"

class UAbilitySystemComponent;
class UBaruCoreAttributeSet;
class UBaruPlayerAttributeSet;
class UBaruHealthComponent;

/**
 * 플레이어 상태 및 ASC, AttributeSet 소유
 * Todo : 캐릭터가 죽은 뒤에 아이디는 그대로 유지하고 스테이터스 및 장비를 초기화하는 로직이 필요
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
    
    UBaruCoreAttributeSet* GetCoreAttributeSet() const { return CoreAttributeSet; }
    UBaruPlayerAttributeSet* GetPlayerAttributeSet() const { return PlayerAttributeSet; }
    
    // HealthComponent Getter
    UFUNCTION(BlueprintPure, Category = "BARU|PlayerState")
    UBaruHealthComponent* GetHealthComponent() const { return HealthComponent; }

    // Getter & Setter
    UFUNCTION(BlueprintPure, Category = "BARU|PlayerState")
    bool IsReady() const { return bIsReady; }

    UFUNCTION(BlueprintPure, Category = "BARU|PlayerState")
    bool IsDBNO() const { return bIsDBNO; }

    UFUNCTION(BlueprintPure, Category = "BARU|PlayerState")
    float GetSanity() const { return Sanity; }

    UFUNCTION(BlueprintPure, Category = "BARU|PlayerState")
    float GetHealth() const { return Health; }

    UFUNCTION(BlueprintPure, Category = "BARU|PlayerState")
    float GetMaxHealth() const { return MaxHealth; }

    // Server RPCs
    UFUNCTION(Server, Reliable, WithValidation, Category = "BARU|PlayerState")
    void Server_SetReadyStatus(bool bNewReady);

    UFUNCTION(Server, Reliable, WithValidation, Category = "BARU|PlayerState")
    void Server_UpdateNickname(const FString& NewNickname);
    
    UFUNCTION(BlueprintAuthorityOnly, Category = "BARU|PlayerState")
    void SetDBNOState(bool bNewDBNO);

    UFUNCTION(BlueprintAuthorityOnly, Category = "BARU|PlayerState")
    void SetSanityValue(float NewSanity);

    UFUNCTION(BlueprintAuthorityOnly, Category = "BARU|PlayerState")
    void SetHealthValue(float NewHealth);

    UFUNCTION(BlueprintAuthorityOnly, Category = "BARU|PlayerState")
    void SetMaxHealthValue(float NewMaxHealth);
    
public:
    // UI 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "BARU|PlayerState|Event")
    FOnBaruReadyStatusChanged OnReadyStatusChanged;

    UPROPERTY(BlueprintAssignable, Category = "BARU|PlayerState|Event")
    FOnBaruDBNOStatusChanged OnDBNOStatusChanged;

    UPROPERTY(BlueprintAssignable, Category = "BARU|PlayerState|Event")
    FOnBaruSanityChanged OnSanityChanged;

    UPROPERTY(BlueprintAssignable, Category = "BARU|PlayerState|Event")
    FOnBaruHealthChanged OnHealthChanged;
    
protected:
    // GAS Components 부착
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|GAS")
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|GAS")
    TObjectPtr<UBaruCoreAttributeSet> CoreAttributeSet;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|GAS")
    TObjectPtr<UBaruPlayerAttributeSet> PlayerAttributeSet;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
    TObjectPtr<UBaruHealthComponent> HealthComponent;
    
    // Replicated Properties & RepNotifies
    UPROPERTY(ReplicatedUsing = OnRep_IsReady, VisibleInstanceOnly, Category = "BARU|State")
    bool bIsReady = false;

    UPROPERTY(ReplicatedUsing = OnRep_IsDBNO, VisibleInstanceOnly, Category = "BARU|State")
    bool bIsDBNO = false;

    UPROPERTY(ReplicatedUsing = OnRep_Sanity, VisibleInstanceOnly, Category = "BARU|State")
    float Sanity = 100.0f;

    UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleInstanceOnly, Category = "BARU|State")
    float Health = 100.0f;

    UPROPERTY(ReplicatedUsing = OnRep_MaxHealth, VisibleInstanceOnly, Category = "BARU|State")
    float MaxHealth = 100.0f;

    UFUNCTION()
    virtual void OnRep_IsReady();

    UFUNCTION()
    virtual void OnRep_IsDBNO();

    UFUNCTION()
    virtual void OnRep_Sanity();

    UFUNCTION()
    virtual void OnRep_Health();

    UFUNCTION()
    virtual void OnRep_MaxHealth();
};