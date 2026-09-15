#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"  
#include "InputActionValue.h"
#include "Engine/EngineTypes.h"     
#include "Interfaces/CombatInterface.h" 
#include "Interfaces/InteractableInterface.h"
#include "GameplayCueInterface.h"
#include "Gameplay/Equipment/DataTypes/BaruEquipmentTypes.h"   
#include "BaruCharacter.generated.h"     

// [09.13] 총기 반동 데이터 구조체
USTRUCT(BlueprintType)
struct FBaruRecoilData
{
    GENERATED_BODY()

    // 상향(Pitch) 반동 각도 범위 (최소~최대)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Recoil", meta = (ClampMin = "0.0"))
    float MinPitchRecoil = 0.8f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Recoil", meta = (ClampMin = "0.0"))
    float MaxPitchRecoil = 1.4f;

    // 좌우(Yaw) 반동 각도 범위 (-좌, +우)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Recoil")
    float MinYawRecoil = -0.3f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Recoil")
    float MaxYawRecoil = 0.5f;

    // 반동 회복 속도 (초당 복구되는 각도, 0이면 회복 안 함)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Recoil", meta = (ClampMin = "0.0"))
    float RecoilRecoverySpeed = 10.0f;
};

// 전방 선언 
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class USkeletalMeshComponent; 
class UAbilitySystemComponent;
class UBaruCharacterAnimSet;           
class UBaruEquipmentComponent; 
class UBaruItemInstance;   
class USpotLightComponent;
class UBaruTensionComponent;
class UBaruFootstepComponent;
class UAnimMontage;
struct FOnAttributeChangeData;
enum class EBaruInteractionHoldEndReason : uint8;

UCLASS()
class BARUGAME_API ABaruCharacter : public ACharacter,public IAbilitySystemInterface, public ICombatInterface , public IInteractableInterface,public IGameplayCueInterface
{
    GENERATED_BODY()

public:
    ABaruCharacter();
    
    // [09.13] 반동 회복 처리를 위한 Tick 오버라이드
    virtual void Tick(float DeltaSeconds) override;
    virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
    virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

    // [09.13] 사격 시 화면 반동 트리거 함수
    UFUNCTION(BlueprintCallable, Category = "BARU|Combat")
    void ApplyRecoil(const FBaruRecoilData& InRecoilData);
    
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override; // [추가]
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;         
    virtual void HandleGameplayCue(UObject* Self, FGameplayTag GameplayCueTag, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;
    
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    
    UFUNCTION(BlueprintPure, Category = "BARU|Movement")
    bool IsSprinting() const { return bIsSprinting; }
    
    UFUNCTION(BlueprintPure, Category = "BARU|Headlight")
    bool IsHeadlightOn() const { return bHeadlightOn; }
    
    //수정 서버재검증 트레이스까지 선을 그리면 지저분해짐
    UFUNCTION(BlueprintCallable, Category = "BARU|Combat") 
    bool PerformLineTrace(FHitResult& OutHitResult, float TraceDistance = 1000.0f, bool bDrawDebug = false);

    // 서버 상호작용 요청 RPC 예시
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_ProcessInteraction(const FHitResult& HitResult);
    
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_StopInteraction();
    
    virtual void PossessedBy(AController* NewController) override;
    virtual void OnRep_PlayerState() override;

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void OnRep_Controller() override;
    
    
    // [추가] DBNO(다운) 진입. 서버 전용.
    //   BaruCoreAttributeSet 이 첫 체력 0 도달 시 호출합니다.
    UFUNCTION(BlueprintCallable, Category = "BARU|Combat")
    void EnterDBNO(AActor* DownCauser);

    // [추가] DBNO 해제(소생). 서버 전용.
    UFUNCTION(BlueprintCallable, Category = "BARU|Combat")
    void ReviveFromDBNO(float HealthRatio = 0.3f);
    
    // [추가] CombatInterface 구현 함수 
    virtual void Die_Implementation(AActor* Killer) override;
    virtual bool IsDead_Implementation() const override;
    
    // [추가] InteractableInterface 구현 — 다운된 팀원 소생
    virtual bool     CanInteract_Implementation(APawn* Interactor) const override;
    virtual FText    GetInteractPromptText_Implementation(APawn* Interactor) const override;
    virtual FGameplayTag GetInteractionTag_Implementation() const override;
    virtual float    GetInteractionDuration_Implementation() const override;
    virtual void     ExecuteInteraction_Implementation(APawn* Interactor) override;
    
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
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Combat|Death")
    float DeathRagdollStartRatio = 0.7f;

    FTimerHandle DeathRagdollTimerHandle;

    // ★[추가 09.15] 사망 시 몸을 물리(래그돌)로 전환 — 시체가 널브러지게 (각 PC 에서 연출 전용)
    void StartDeathRagdoll();
    void InitAbilityActorInfo();

    // 이동 및 시점 회전 처리 함수
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    
    void Input_Jump();          
    void Input_StopJumping();   
    void Input_Interact(); 
    void Input_StopInteract();
    void Input_ToggleHeadlight();
    
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_SetHeadlightOn(bool bNewOn);

    UFUNCTION()
    void OnRep_HeadlightOn();
    void UpdateHeadlightVisual();
    
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
    void HandleReloadingTagChanged(const FGameplayTag Tag, int32 NewCount);

    UFUNCTION()
    void OnRep_IsDead();      

    void NotifyGameModeOfDeath(); 
    
    // [추가] DBNO 상태가 바뀌었을 때 이동·연출을 갱신합니다.
    //   PlayerState 의 OnDBNOStatusChanged 에 바인딩되어 서버·클라 모두에서 호출됩니다.
    UFUNCTION()
    void HandleDBNOStatusChanged(bool bNewDBNO);

    // 블리드아웃(방치 시 사망) 타이머
    FTimerHandle BleedOutTimerHandle;

    void OnBleedOutExpired();

    // [추가] 다운 상태로 버틸 수 있는 시간(초)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Combat")
    float BleedOutDuration = 60.0f;
    
    // [추가] 소생에 걸리는 시간(초). F 를 이만큼 누르고 있어야 합니다.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Combat")
    float ReviveDuration = 5.0f;

    // [추가] 소생 시 회복 비율
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Combat")
    float ReviveHealthRatio = 0.3f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Anim")
    TMap<FGameplayTag, TObjectPtr<UAnimMontage>> FireMontageByCue;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Anim")
    TMap<EBaruEquipmentSlot, TObjectPtr<UAnimMontage>> ReloadMontageBySlot;
    
    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> CurrentReloadMontage;
    
    // [추가] 다운 중에도 사망 연출과 구분되도록 BP 훅을 열어둡니다.
    UFUNCTION(BlueprintImplementableEvent, Category = "BARU|Combat")
    void OnDBNOCosmetic(bool bNewDBNO);
    
    // [추가] 진행 중인 홀드 상호작용 (서버 전용 상태)
    UPROPERTY(Transient)
    TWeakObjectPtr<AActor> PendingInteractTarget;

    FTimerHandle InteractionTimerHandle;
    
    // [추가] 홀드 도중 거리·대상 상태를 0.1초마다 확인하는 타이머
    FTimerHandle InteractionHoldCheckTimerHandle;

    // [추가] 지금 진행 중인 게 "홀드"인지 (09.13 에 즉시 실행 대상도 캐싱하게 바뀌어서 구분 필요)
    bool bInteractionHoldActive = false;

    void CompletePendingInteraction();
    void CancelPendingInteraction();

    // [추가 ] 홀드 검사·종료 (서버 전용)
    void CheckPendingInteractionHold();
    bool ValidatePendingInteractionHold(EBaruInteractionHoldEndReason& OutFailReason);
    void EndInteractionHold(EBaruInteractionHoldEndReason Reason);



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
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Footstep")
    TObjectPtr<UBaruFootstepComponent> FootstepComponent;
    
    // [추가 ] 헤드라이트. 세부 값은 BP 에서 조정가능
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Headlight")
    TObjectPtr<USpotLightComponent> Headlight;

    // 켜짐 상태. 복제되어야 다른 플레이어 화면에서도 내 불빛이 보임
    UPROPERTY(ReplicatedUsing = OnRep_HeadlightOn, VisibleInstanceOnly, BlueprintReadOnly, Category = "BARU|Headlight")
    bool bHeadlightOn = false;
    
    // 긴장도 및 심박수 연출 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
    TObjectPtr<UBaruTensionComponent> TensionComponent;
    
    // 크라우치를 넣을 때 반드시 다시 손대게 됩니다
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Camera")
    float CameraEyeHeight = 60.0f;
    
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Camera")
    FName CameraFollowSocketName = TEXT("head");
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Camera")
    FVector CameraFollowOffset = FVector(10.0f, 0.0f, 5.0f);
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Camera")
    float CameraFollowInterpSpeed = 12.0f;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Camera")
    bool bCameraAlwaysFollowHead = true;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Camera")
    float CameraLookDownForwardPush = 20.0f;
    
    FVector DefaultCameraRelativeLocation = FVector::ZeroVector;

    // ★[추가 09.15] 앉기·다운 시 카메라 위치 갱신 (로컬 전용, Tick 에서 호출)
    void UpdateCameraFollow(float DeltaSeconds);
    
    FTimerHandle WeaponShadowTimerHandle;
    void HideLocalWeaponShadows();
    
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
    
    //헤드라이트 토글 (L 키)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
    TObjectPtr<UInputAction> HeadlightAction;
    
    // [추가] 달리기 (Shift, Hold)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
    TObjectPtr<UInputAction> SprintAction;

    // [추가] 앉기 (Ctrl, Toggle)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
    TObjectPtr<UInputAction> CrouchAction;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Interaction")
    float InteractionHoldBreakTolerance = 100.0f;

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

    FDelegateHandle ReloadingTagChangedHandle;
    
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
    
    FDelegateHandle CarryWeightChangedHandle;
    FDelegateHandle MaxCarryWeightChangedHandle;
    void HandleCarryWeightChanged(const FOnAttributeChangeData& ChangeData);

    UPROPERTY()
    TObjectPtr<AActor> LastKiller; // [추가] 사망처리를 다음 틱으로 넘길때 임시보관
    
public:
    // [추가] 다운 상태에서 추가 피격을 받았을 때 호출 (출혈시간 단축)
    void NotifyHitWhileDBNO(float DamageAmount, AActor* Attacker);

protected:
    // [추가] 다운 직후 무적 해제 타이머 핸들
    FTimerHandle DBNOImmunityTimerHandle;

    // [추가] 무적 해제 콜백
    void EndDBNOImmunity();

    // 다운 직후 무적 지속 시간 (기본 2.5초)
    UPROPERTY(EditDefaultsOnly, Category = "BARU|Combat")
    float DBNOImmunityDuration = 2.5f;

    // 다운 중 1회 피격 시 차감될 출혈 시간 (초 단위, 기본 15초)
    UPROPERTY(EditDefaultsOnly, Category = "BARU|Combat")
    float DBNODamageBleedReduction = 2.0f;
    
    // [09.13] R키 수동 재장전 InputAction 에셋 포인터
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
    TObjectPtr<UInputAction> ReloadAction;
    
    // [09.13] 수동 재장전 입력 핸들러
    void Input_Reload();
    
    
    // [09.13] 자연 회복 파이프라인 설정 및 타이머 핸들
    UPROPERTY(EditDefaultsOnly, Category = "BARU|Combat|Regen")
    float HealthRegenDelay = 10.0f;
    
    UPROPERTY(EditDefaultsOnly, Category = "BARU|Combat|Regen")
    float MaxRegenHealth = 80.0f;

    UPROPERTY(EditDefaultsOnly, Category = "BARU|Combat|Regen")
    float HealthRegenRatePerSecond = 5.0f;

    UPROPERTY(EditDefaultsOnly, Category = "BARU|Combat|Regen")
    float HealthRegenTickInterval = 0.5f;

    FTimerHandle HealthRegenDelayTimerHandle;
    FTimerHandle HealthRegenTickTimerHandle;

    void StartHealthRegenDelay();
    void OnHealthRegenDelayExpired();
    void TickHealthRegen();
    void StopHealthRegen();
    
private:
    // [09.13] Early-Out 반동 복구 상태 변수
    float RemainingRecoilRecoveryPitch = 0.0f;
    float CurrentRecoilRecoverySpeed = 0.0f;

    
public:
    // [09.13] 조준(ADS) 집중 상태 온/오프 (GAS Ability 및 캐릭터 직접 입력 공용)
    UFUNCTION(BlueprintCallable, Category = "BARU|Camera")
    void SetAiming(bool bNewAiming);

    UFUNCTION(BlueprintPure, Category = "BARU|Camera")
    bool IsAiming() const { return bIsAiming; }
  
protected:
    // =========================================================================
    // 조준(ADS) & 호러 터널비전 연출 파라미터
    // =========================================================================
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Camera|Aim")
    float DefaultFOV = 90.0f;

    // 왜곡이 생기지 않도록 살짝만 축소 (78~80도 권장)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Camera|Aim")
    float AimFOV = 78.0f;

    // 평상시 외곽 어두움 -> 조준 시 짙은 터널 비전
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Camera|Aim")
    float DefaultVignette = 0.35f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Camera|Aim")
    float AimVignette = 0.75f;

    // 조준 시 렌즈 외곽 색수차 왜곡 (공포/어지러움 극대화)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Camera|Aim")
    float DefaultFringe = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Camera|Aim")
    float AimFringe = 1.5f;

    // 줌 및 후처리 보간 속도
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Camera|Aim")
    float AimInterpSpeed = 12.0f;

    // 우클릭 조준 InputAction 에셋
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
    TObjectPtr<UInputAction> AimAction;

    void Input_AimStart();
    void Input_AimStop();

private:
    bool bIsAiming = false;
    float CurrentTargetFOV = 90.0f;
    float CurrentTargetVignette = 0.35f;
    float CurrentTargetFringe = 0.0f;

    void UpdateAimingEffects(float DeltaSeconds);
};