#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "BaruPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruReadyStatusChanged, bool, bIsReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruDBNOStatusChanged, bool, bIsDBNO);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruSanityChanged, float, NewSanity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruDeadStatusChanged, bool, bIsDead); // [추가]

class UAbilitySystemComponent;
class UBaruAbilitySystemComponent;      // [추가]
class UBaruCoreAttributeSet;
class UBaruPlayerAttributeSet;
class UBaruHealthComponent;
class UBaruInventoryComponent;      // item-iven 연동
struct FOnAttributeChangeData;          // [추가] Sanity 어트리뷰트 콜백용

//플레이어 상태 및 ASC, AttributeSet 소유
//Todo : 캐릭터가 죽은 뒤에 아이디는 그대로 유지하고 스테이터스 및 장비를 초기화하는 로직이 필요
 
UCLASS()
class BARUGAME_API ABaruPlayerState : public APlayerState, public IAbilitySystemInterface
{
    GENERATED_BODY()
    
public:
    ABaruPlayerState();
    
    
    virtual void PostInitializeComponents() override;                        // [추가]
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;  // [추가]
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    
    // [추가] 레벨 이동(Seamless Travel) 시 값 인수인계
    virtual void CopyProperties(APlayerState* PlayerState) override;
    // IAbilitySystemInterface
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    
    // [추가] Baru 전용 ASC 타입 그대로 반환 (PlayerController 의 ProcessAbilityInput 호출용)
    UBaruAbilitySystemComponent* GetBaruAbilitySystemComponent() const { return AbilitySystemComponent; }
    
    UBaruCoreAttributeSet* GetCoreAttributeSet() const { return CoreAttributeSet; }
    UBaruPlayerAttributeSet* GetPlayerAttributeSet() const { return PlayerAttributeSet; }
    
    // HealthComponent Getter
    UFUNCTION(BlueprintPure, Category = "BARU|PlayerState")
    UBaruHealthComponent* GetHealthComponent() const { return HealthComponent; }
    
    UFUNCTION(BlueprintPure, Category = "BARU|PlayerState")
    UBaruInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }  //다른사람이 인벤토리에 접근할 수 있게 해주는 getter추가
    
    // Getter & Setter
    UFUNCTION(BlueprintPure, Category = "BARU|PlayerState")
    bool IsReady() const { return bIsReady; }
    
    // [추가] 이 플레이어가 아직 게임에 참여 중인가단일 판단 함수.
    //   GameMode 담당자에게 이걸로 교체해달라고 요청할 예정.
    UFUNCTION(BlueprintPure, Category = "BARU|PlayerState")
    bool IsAlive() const { return !bIsDead && !bIsDBNO; }
    
    UFUNCTION(BlueprintPure, Category = "BARU|PlayerState")
    bool IsDBNO() const { return bIsDBNO; }
    
    // [추가] 사망 여부 / 생존 여부
    UFUNCTION(BlueprintPure, Category = "BARU|PlayerState")
    bool IsDead() const { return bIsDead; }
    
    // [수정] 인라인 → .cpp 이동 (헤더 의존성 제거). 동작은 기존과 동일
    UFUNCTION(BlueprintPure, Category = "BARU|PlayerState")
    float GetSanity() const;
    
    UFUNCTION(BlueprintPure, Category = "BARU|PlayerState")
    float GetHealth() const;

    UFUNCTION(BlueprintPure, Category = "BARU|PlayerState")
    float GetMaxHealth() const;
    
    // Server RPCs
    UFUNCTION(Server, Reliable, WithValidation, Category = "BARU|PlayerState")
    void Server_SetReadyStatus(bool bNewReady);

    UFUNCTION(Server, Reliable, WithValidation, Category = "BARU|PlayerState")
    void Server_UpdateNickname(const FString& NewNickname);
    
    UFUNCTION(BlueprintAuthorityOnly, Category = "BARU|PlayerState")
    void SetDBNOState(bool bNewDBNO);

    UFUNCTION(BlueprintAuthorityOnly, Category = "BARU|PlayerState")
    void SetSanityValue(float NewSanity);
    
    // [추가] 서버에서 사망 확정/해제 시 호출.
    // ABaruCharacter::Die_Implementation 이 true 로, PossessedBy(리스폰) 가 false 로 호출
    UFUNCTION(BlueprintAuthorityOnly, Category = "BARU|PlayerState")
    void SetDeadState(bool bNewDead, AActor* Killer = nullptr);

    
public:
    // UI 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "BARU|PlayerState|Event")
    FOnBaruReadyStatusChanged OnReadyStatusChanged;

    UPROPERTY(BlueprintAssignable, Category = "BARU|PlayerState|Event")
    FOnBaruDBNOStatusChanged OnDBNOStatusChanged;

    UPROPERTY(BlueprintAssignable, Category = "BARU|PlayerState|Event")
    FOnBaruSanityChanged OnSanityChanged;
    
    UPROPERTY(BlueprintAssignable, Category = "BARU|PlayerState|Event")   // [추가]
    FOnBaruDeadStatusChanged OnDeadStatusChanged;
    
protected:
    // GAS Components 부착
    // [수정] UAbilitySystemComponent → UBaruAbilitySystemComponent.
    //  순정 ASC 를 쓰고 있어서 태그 기반 어빌리티 입력(AbilityInputTagPressed 등)을
    //  플레이어가 전혀 못 쓰는 상태였기때문 몬스터는 이미 Baru 버전을 쓰고있음
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|GAS")
    TObjectPtr<UBaruAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|GAS")
    TObjectPtr<UBaruCoreAttributeSet> CoreAttributeSet;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|GAS")
    TObjectPtr<UBaruPlayerAttributeSet> PlayerAttributeSet;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
    TObjectPtr<UBaruHealthComponent> HealthComponent;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Inventory")
    TObjectPtr<UBaruInventoryComponent> InventoryComponent;             //인벤토리 컴포넌트를 소유하도록 보관하는 변수
    
    // Replicated Properties & RepNotifies
    UPROPERTY(ReplicatedUsing = OnRep_IsReady, VisibleInstanceOnly, Category = "BARU|State")
    bool bIsReady = false;

    UPROPERTY(ReplicatedUsing = OnRep_IsDBNO, VisibleInstanceOnly, Category = "BARU|State")
    bool bIsDBNO = false;

    UPROPERTY(ReplicatedUsing = OnRep_IsDead, VisibleInstanceOnly, Category = "BARU|State")   // ★[추가]
    bool bIsDead = false;

    // [삭제] float Sanity = 100.0f; 중복

    UFUNCTION()
    virtual void OnRep_IsReady();

    UFUNCTION()
    virtual void OnRep_IsDBNO();

    UFUNCTION()
    virtual void OnRep_IsDead();      // [추가]

    // [삭제] OnRep_Sanity()
    // [추가] Sanity 어트리뷰트 변경 → UI 델리게이트 브로드캐스트
    void HandleSanityChanged(const FOnAttributeChangeData& ChangeData);

private:
    FDelegateHandle SanityChangedHandle;   // [추가]
};