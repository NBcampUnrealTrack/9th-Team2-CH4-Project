#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/SoftObjectPtr.h"
#include "BaruElevatorActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class ABaruGameState;
class APawn;

/**
 * 회사 로비 및 던전에 배치되는 레벨 전환용 엘리베이터 발판 액터
 * - 전원 탑승 시 5초 카운트다운 시작
 * - 도중 이탈 시 자동 카운트다운 취소
 * - TSoftObjectPtr<UWorld> 기반으로 목적지 맵을 에디터에서 자유롭게 지정
 */
UCLASS()
class BARUGAME_API ABaruElevatorActor : public AActor
{
    GENERATED_BODY()
    
public: 
    ABaruElevatorActor();

    virtual void BeginPlay() override;

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

    /** 현재 접속/생존 중인 전원 탑승 여부 검사 */
    bool CheckAllPlayersBoarded();

    /** 카운트다운 시작/중단/완료 */
    void StartCountdown();
    void CancelCountdown();
    void UpdateCountdownTick();
    void OnCountdownCompleted();

protected:
    // ==============================================================================
    // Components
    // ==============================================================================
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
    TObjectPtr<USceneComponent> RootSceneComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
    TObjectPtr<UStaticMeshComponent> PlatformMeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
    TObjectPtr<UBoxComponent> BoardingTriggerBox;

    // ==============================================================================
    // Configurable Settings (하드코딩 배제)
    // ==============================================================================
    /** 이동할 목적지 레벨 에셋 (에디터 디테일 패널에서 드래그 & 드롭 지정) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Elevator|Destination")
    TSoftObjectPtr<UWorld> DestinationLevel;

    /** 전원 탑승 후 출발까지의 대기 시간 (기본 5초) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Elevator|Settings", meta = (ClampMin = "1.0"))
    float CountdownDuration = 5.0f;

    // ==============================================================================
    // Internal State
    // ==============================================================================
    /** 현재 발판 위에 서 있는 유효 플레이어 목록 */
    UPROPERTY(Transient)
    TSet<TWeakObjectPtr<APawn>> BoardedPlayers;

    UPROPERTY(Transient)
    TObjectPtr<ABaruGameState> CachedGameState;

    FTimerHandle CountdownTimerHandle;
    float RemainingCountdown = 0.0f;
    bool bIsCountingDown = false;
    
    // 레벨 진입 후 엘리베이터가 재작동하기까지 필요한 대기 시간 (초)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Elevator|Settings")
    float ArrivalLockoutDuration = 10.0f;

    // 엘리베이터 작동 가능 플래그 (레벨 시작하자마자 다음 레벨로 전환하는 것을 방지)
    bool bIsElevatorArmed = false;

    //락아웃 해제 함수
    void EnableElevatorActivation();
};