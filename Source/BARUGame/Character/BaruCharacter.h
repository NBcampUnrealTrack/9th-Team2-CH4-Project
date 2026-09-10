#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"  
#include "InputActionValue.h"
#include "Engine/EngineTypes.h"     
#include "Interfaces/CombatInterface.h" 
#include "Gameplay/Equipment/DataTypes/BaruEquipmentTypes.h"   
#include "BaruCharacter.generated.h"     

// 전방 선언 
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class USkeletalMeshComponent; 
class UAbilitySystemComponent;
class UBaruCharacterAnimSet;           
class UBaruEquipmentComponent; 
class UBaruItemInstance;   
struct FOnAttributeChangeData;               

UCLASS()
class BARUGAME_API ABaruCharacter : public ACharacter,public IAbilitySystemInterface, public ICombatInterface // [추가] ICombatInterface 상속
{
    GENERATED_BODY()

public:
    ABaruCharacter();
    
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override; // [추가]
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;                             // [추가]
    
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    
    UFUNCTION(BlueprintPure, Category = "BARU|Movement")
    bool IsSprinting() const { return bIsSprinting; }
    //수정 서버재검증 트레이스까지 선을 그리면 지저분해짐
    UFUNCTION(BlueprintCallable, Category = "BARU|Combat") 
    bool PerformLineTrace(FHitResult& OutHitResult, float TraceDistance = 1000.0f, bool bDrawDebug = false);

    // 서버 상호작용 요청 RPC 예시
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_ProcessInteraction(const FHitResult& HitResult);
    
    virtual void PossessedBy(AController* NewController) override;
    virtual void OnRep_PlayerState() override;

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void OnRep_Controller() override;
    
    // [추가] CombatInterface 구현 함수 
    virtual void Die_Implementation(AActor* Killer) override;
    virtual bool IsDead_Implementation() const override;
    
    // [추가] 아래 7개는 구현이 없어서 UHT기본스터입이 0 / FALSE를 반환하고 있던 함수들
    virtual bool  IsDBNO_Implementation() const override;
    virtual bool  IsGroggy_Implementation() const override;
    virtual float GetCurrentHealth_Implementation() const override;
    virtual float GetMaxHealth_Implementation() const override;
    virtual float GetSuppressionRatio_Implementation() const override;
    virtual void  ApplyCombatDamage_Implementation(float DamageAmount, const FHitResult& HitResult, AActor* DamageCauser, AController* InstigatedBy) override;
    virtual void  BreakBodyPart_Implementation(FName BoneName, float Damage) override;
    
    // 클라이언트 IMC 활성화 보장
    virtual void PawnClientRestart() override;

    // [추가] 래그돌/사망 몽타주 같은 연출은 C++이 아니라 BP에서 붙이도록 훅만 열어둠
    //   서버/클라 각자 로컬 호출이라 RPC 낭비가 없기때문에 추가
    UFUNCTION(BlueprintImplementableEvent, Category = "BARU|Combat")
    void OnDeathCosmetic();
    
protected:
    virtual void BeginPlay() override;
    void InitAbilityActorInfo();

    // 이동 및 시점 회전 처리 함수
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    
    void Input_Jump();          
    void Input_StopJumping();   
    void Input_Interact();      
    
    void Input_Fire();            
    void Input_SprintStart();       
    void Input_SprintStop();       
    void Input_ToggleCrouch();   
    void Input_StopFire();              // [추가] 연사 중지
    void Input_SelectPrimaryWeapon();   // [추가] 주무기 전환
    void Input_SelectSecondaryWeapon(); // [추가] 보조무기 전환
    
    void HandleWeaponSlotInput(EBaruEquipmentSlot DesiredSlot);
    
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_RequestSelectWeaponSlot(EBaruEquipmentSlot DesiredSlot);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_RequestUnequipWeapon(EBaruEquipmentSlot WeaponSlot);
    
    UBaruItemInstance* FindInventoryWeaponForSlot(EBaruEquipmentSlot DesiredSlot) const;

    // [추가] 스프린트 상태가 복제되면 각 클라에서 이동 속도를 갱신
    UFUNCTION()
    void OnRep_IsSprinting();

    // [추가] 클라이언트의 달리기 요청을 서버가 확정
    //   이동 속도는 서버가 정해야 위치 보정
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_SetSprinting(bool bNewSprinting);

    // ★[추가] 현재 상태에 맞는 이동 속도를 계산해 적용합니다. 서버·클라 공통.
    void UpdateMaxWalkSpeed();
    
    void HandleMoveSpeedChanged(const FOnAttributeChangeData& ChangeData);  // [추가]

    UFUNCTION()
    void OnRep_IsDead();      

    void NotifyGameModeOfDeath(); 


protected:
    // 1인칭 카메라 
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Camera")
    TObjectPtr<UCameraComponent> FollowCamera;
    
    // 1인칭 팔/무기 전용 메쉬 
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Mesh")
    TObjectPtr<USkeletalMeshComponent> Mesh1P;
    
    //인벤토리 컴포넌트 
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Equipment")
    TObjectPtr<UBaruEquipmentComponent> EquipmentComponent;
    
    // 크라우치를 넣을 때 반드시 다시 손대게 됩니다
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Camera")
    float CameraEyeHeight = 60.0f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Animation")
    TObjectPtr<UBaruCharacterAnimSet> AnimSet;  //이 캐릭터가 사용하는 몽타주 모음
    
    //역할 확정 imc등록 책임은 전부 캐릭터 헤더에서 처리하기로 정함  baruplayercontroller에 있는 mappingcontext는 중복이라 삭제
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;
    
    // 인풋 액션 에셋 할당용 포인터 변수 
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
    TObjectPtr<UInputAction> JumpAction;
    
    //  F키 상호작용 액션 
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
    TObjectPtr<UInputAction> InteractAction;
    
    // [추가] 발사 (좌클릭)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
    TObjectPtr<UInputAction> FireAction;
    
    // [추가] 무기 슬롯 전환 (1번 / 2번 키)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
    TObjectPtr<UInputAction> SelectPrimaryWeaponAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
    TObjectPtr<UInputAction> SelectSecondaryWeaponAction;

    // [추가] 달리기 (Shift, Hold)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
    TObjectPtr<UInputAction> SprintAction;

    // [추가] 앉기 (Ctrl, Toggle)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
    TObjectPtr<UInputAction> CrouchAction;

    // 상호작용 트레이스 설정
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Interaction")
    TEnumAsByte<ECollisionChannel> InteractionTraceChannel = ECC_GameTraceChannel3; 
    
    // Interaction

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Interaction")
    float InteractionTraceDistance = 300.0f;

    // 서버 재검증 시 네트워크 지연으로 인한 위치 오차 허용치
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Interaction")
    float InteractionLagTolerance = 100.0f;
    
    // [추가] 달리기 속도 배율
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Movement")
    float SprintSpeedMultiplier = 1.6f;

    // [추가] 앉았을 때 이동 속도 
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Movement")
    float CrouchedWalkSpeed = 200.0f;

    // [추가] 달리는 중인지. 복제되어야 다른 사람 화면에서도 속도가 맞음
    UPROPERTY(ReplicatedUsing = OnRep_IsSprinting, VisibleInstanceOnly, BlueprintReadOnly, Category = "BARU|Movement")
    bool bIsSprinting = false;
    
    UPROPERTY(ReplicatedUsing = OnRep_IsDead, VisibleInstanceOnly, BlueprintReadOnly, Category = "BARU|Combat")
    bool bIsDead = false;

private:
    //MoveSpeed 델리게이트 중복 바인딩 방지용
    TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
    // [추가] MoveSpeed 어트리뷰트가 준 기본 속도. 스프린트 배율의 기준값
    float BaseWalkSpeed = 450.0f;
    FDelegateHandle MoveSpeedChangedHandle;

    UPROPERTY()
    TObjectPtr<AActor> LastKiller; // [추가] 사망처리를 다음 틱으로 넘길때 임시보관
};