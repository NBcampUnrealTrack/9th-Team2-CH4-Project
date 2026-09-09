#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/SoftObjectPtr.h"
#include "Interfaces/InteractableInterface.h"
#include "BaruElevatorActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class ABaruGameState;
class APawn;

UENUM(BlueprintType)
enum class EBaruElevatorTriggerType : uint8
{
    AutoOnAllBoarded UMETA(DisplayName = "Auto When All Alive Boarded"), // 전원 탑승 시 자동 출발
    ManualInteract   UMETA(DisplayName = "Manual Button Interaction")    // 전원 탑승 후 콘솔 스위치 조작 필요
};

/**
 * 회사 로비 및 지하 탐사 구역 레벨 전환용 엘리베이터 액터
 * - IInteractableInterface 구현 (수동 버튼 상호작용 지원)
 * - 네트워크 복제(RepNotify) 기반 클라이언트 도어/사운드 연출 동기화
 * - GameMode 이중 딜레이 방지 연동
 */
UCLASS()
class BARUGAME_API ABaruElevatorActor : public AActor, public IInteractableInterface
{
    GENERATED_BODY()
    
public: 
    ABaruElevatorActor();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginPlay() override;

    // ==============================================================================
    // IInteractableInterface 구현부 (콘솔 스위치 수동 작동용)
    // ==============================================================================
    virtual bool CanInteract_Implementation(APawn* Interactor) const override;
    virtual FText GetInteractPromptText_Implementation(APawn* Interactor) const override;
    virtual FGameplayTag GetInteractionTag_Implementation() const override;
    virtual float GetInteractionDuration_Implementation() const override;
    virtual void ExecuteInteraction_Implementation(APawn* Interactor) override;

protected:
    UFUNCTION()
    void HandleTriggerBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );

    UFUNCTION()
    void HandleTriggerEndOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex
    );

    bool CheckAllPlayersBoarded() const;

    void StartCountdown();
    void CancelCountdown();
    void UpdateCountdownTick();
    void OnCountdownCompleted();

    UFUNCTION()
    void OnRep_IsCountingDown();

    UFUNCTION()
    void OnRep_IsDeparted();

    // 블루프린트 연출 훅 (클라이언트/서버 공통 호출)
    UFUNCTION(BlueprintImplementableEvent, Category = "BARU|Elevator|Events")
    void BP_OnCountdownStarted(float Duration);

    UFUNCTION(BlueprintImplementableEvent, Category = "BARU|Elevator|Events")
    void BP_OnCountdownCancelled();

    UFUNCTION(BlueprintImplementableEvent, Category = "BARU|Elevator|Events")
    void BP_OnDeparted();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
    TObjectPtr<USceneComponent> RootSceneComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
    TObjectPtr<UStaticMeshComponent> PlatformMeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
    TObjectPtr<UBoxComponent> BoardingTriggerBox;

    /** 상호작용 가능한 엘리베이터 조종 콘솔 메쉬 (선택 사항) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
    TObjectPtr<UStaticMeshComponent> ConsoleSwitchMesh;

    // ==============================================================================
    // Configurable Settings
    // ==============================================================================
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Elevator|Settings")
    EBaruElevatorTriggerType TriggerType = EBaruElevatorTriggerType::AutoOnAllBoarded;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Elevator|Destination")
    TSoftObjectPtr<UWorld> DestinationLevel;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Elevator|Settings", meta = (ClampMin = "1.0"))
    float CountdownDuration = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Elevator|Settings")
    float ArrivalLockoutDuration = 5.0f;

    // ==============================================================================
    // Replicated State
    // ==============================================================================
    UPROPERTY(ReplicatedUsing = OnRep_IsCountingDown, BlueprintReadOnly, Category = "BARU|Elevator|State")
    bool bIsCountingDown = false;

    UPROPERTY(ReplicatedUsing = OnRep_IsDeparted, BlueprintReadOnly, Category = "BARU|Elevator|State")
    bool bIsDeparted = false;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "BARU|Elevator|State")
    float RemainingCountdown = 0.0f;

private:
    UPROPERTY(Transient)
    TSet<TWeakObjectPtr<APawn>> BoardedPlayers;

    UPROPERTY(Transient)
    TObjectPtr<ABaruGameState> CachedGameState;

    FTimerHandle CountdownTimerHandle;
    FTimerHandle LockoutTimerHandle;
    bool bIsElevatorArmed = false;

    void EnableElevatorActivation();
};