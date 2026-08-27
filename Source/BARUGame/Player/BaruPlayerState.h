#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
// [수정] CoreAttributeSet 접근을 위해 헤더 포함
#include "AbilitySystem/Attributes/BaruCoreAttributeSet.h" 

// UI Binding Delegate (generated.h 위에 선언)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruReadyStatusChanged, bool, bIsReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruDBNOStatusChanged, bool, bIsDBNO);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruSanityChanged, float, NewSanity);
// [삭제] FOnBaruHealthChanged는 HealthComponent가 전담하므로 PlayerState에서는 삭제합니다.

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

    // [수정] PlayerState의 자체 변수를 반환하지 않고 GAS(CoreAttributeSet)의 값을 직접 반환하도록 수정
    UFUNCTION(BlueprintPure, Category = "BARU|PlayerState")
    float GetHealth() const { return CoreAttributeSet ? CoreAttributeSet->GetHealth() : 0.0f; }

    UFUNCTION(BlueprintPure, Category = "BARU|PlayerState")
    float GetMaxHealth() const { return CoreAttributeSet ? CoreAttributeSet->GetMaxHealth() : 0.0f; }
    
    // Server RPCs
    UFUNCTION(Server, Reliable, WithValidation, Category = "BARU|PlayerState")
    void Server_SetReadyStatus(bool bNewReady);

    UFUNCTION(Server, Reliable, WithValidation, Category = "BARU|PlayerState")
    void Server_UpdateNickname(const FString& NewNickname);
    
    UFUNCTION(BlueprintAuthorityOnly, Category = "BARU|PlayerState")
    void SetDBNOState(bool bNewDBNO);

    UFUNCTION(BlueprintAuthorityOnly, Category = "BARU|PlayerState")
    void SetSanityValue(float NewSanity);

    // [삭제] SetHealthValue, SetMaxHealthValue 함수 삭제 (GAS AttributeSet에서 전담)
    
public:
    // UI 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "BARU|PlayerState|Event")
    FOnBaruReadyStatusChanged OnReadyStatusChanged;

    UPROPERTY(BlueprintAssignable, Category = "BARU|PlayerState|Event")
    FOnBaruDBNOStatusChanged OnDBNOStatusChanged;

    UPROPERTY(BlueprintAssignable, Category = "BARU|PlayerState|Event")
    FOnBaruSanityChanged OnSanityChanged;

    // [삭제] OnHealthChanged 델리게이트 삭제 (HealthComponent 사용)
    
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

    //  float Health, MaxHealth 변수 삭제 (GAS와 중복됨)

    UFUNCTION()
    virtual void OnRep_IsReady();

    UFUNCTION()
    virtual void OnRep_IsDBNO();

    UFUNCTION()
    virtual void OnRep_Sanity();

    //  OnRep_Health(), OnRep_MaxHealth() 함수 삭제
};