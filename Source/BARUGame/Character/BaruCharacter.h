#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"  //IAbilitySystemInterface 상속
#include "InputActionValue.h"
#include "Engine/EngineTypes.h"     //ECollisionChannel(전용 트레이스 채널)
#include "Interfaces/CombatInterface.h" 
#include "BaruCharacter.generated.h"     

// 전방 선언 
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class USkeletalMeshComponent; //  Mesh1P를 위한 전방 선언
class UAbilitySystemComponent;               // 
struct FOnAttributeChangeData;               //  MoveSpeed 어트리뷰트 콜백용

UCLASS()
class BARUGAME_API ABaruCharacter : public ACharacter,public IAbilitySystemInterface, public ICombatInterface // [추가] ICombatInterface 상속
{
    GENERATED_BODY()

public:
    ABaruCharacter();
    
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override; // [추가]
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;                             // [추가]
    
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
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
    
    void Input_Jump();          // [추가] 사망 중 점프 차단용 래퍼
    void Input_StopJumping();   // [추가]
    void Input_Interact();      // [추가] F키 상호작용
    
    void HandleMoveSpeedChanged(const FOnAttributeChangeData& ChangeData);  // [추가]

    UFUNCTION()
    void OnRep_IsDead();        // [추가] 클라이언트 사망 연출 진입점

    void NotifyGameModeOfDeath(); // [추가] GameMode 통지(다음 틱 지연 실행)


protected:
    // 1인칭 카메라 
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Camera")
    TObjectPtr<UCameraComponent> FollowCamera;
    
    // 1인칭 팔/무기 전용 메쉬 
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Mesh")
    TObjectPtr<USkeletalMeshComponent> Mesh1P;
    
    // [추가] 카메라 눈높이. 생성자에 60.0f 를 박아두면 README 4-3장 하드코딩 금지에 걸림
    // 크라우치를 넣을 때 반드시 다시 손대게 됩니다
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Camera")
    float CameraEyeHeight = 60.0f;
    
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
    
    // [추가] F키 상호작용 액션 IA_Interact 에셋을 만들어 BP에서 물려줘야함
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
    TObjectPtr<UInputAction> InteractAction;

    // [추가] 상호작용 트레이스 설정
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Interaction")
    TEnumAsByte<ECollisionChannel> InteractionTraceChannel = ECC_GameTraceChannel3; 
    
    // Interaction

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Interaction")
    float InteractionTraceDistance = 300.0f;

    // [추가] 서버 재검증 시 네트워크 지연으로 인한 위치 오차 허용치
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Interaction")
    float InteractionLagTolerance = 100.0f;

    // [추가]  가장 중요 
    //   BaruCoreAttributeSet.cpp 는 SetHealth(0) 을 먼저 하고 그 다음에 Execute_IsDead() 를 묻는데,
    //   기존 IsDead 구현이 "체력 <= 0" 이었기 때문에 항상 true → !IsDead() == false →
    //   Die() 가 단 한 번도 호출되지 않음
    //   AttributeSet 은 제 권한 밖이라 사망 상태를 여기서 따로 들고 해결
    UPROPERTY(ReplicatedUsing = OnRep_IsDead, VisibleInstanceOnly, BlueprintReadOnly, Category = "BARU|Combat")
    bool bIsDead = false;

private:
    // [추가] MoveSpeed 델리게이트 중복 바인딩 방지용
    TWeakObjectPtr<UAbilitySystemComponent> CachedASC;
    FDelegateHandle MoveSpeedChangedHandle;

    UPROPERTY()
    TObjectPtr<AActor> LastKiller; // [추가] 사망처리를 다음 틱으로 넘길때 임시보관
};