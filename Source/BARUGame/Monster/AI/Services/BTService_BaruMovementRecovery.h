

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_BaruMovementRecovery.generated.h"

class APawn;
class UBlackboardComponent;


UCLASS()
class BARUGAME_API UBTService_BaruMovementRecovery : public UBTService
{
    GENERATED_BODY()

public:

    UBTService_BaruMovementRecovery();

protected:

    // Behavior Tree가 실행되는 동안 일정 간격으로 호출
    // 실제 이동 중인데 제자리에 머무는 몬스터를 감지
    virtual void TickNode(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory,
        float DeltaSeconds
    ) override;

private:

    // 현재 위치 주변에서 실제로 이동 가능한 복구 지점을 탐색
    bool FindRecoveryLocation(
        const APawn& ControlledPawn,
        FVector& OutRecoveryLocation
    ) const;

    // 제자리걸음 감지 상태를 초기화
    void ResetMovementObservation(
        const FVector& CurrentLocation
    );

    // 복구 위치를 Blackboard에 기록하고 복구 상태 시작
    void BeginMovementRecovery(
        UBlackboardComponent& BlackboardComponent,
        const FVector& RecoveryLocation,
        const FVector& CurrentLocation
    );

    // 복구 성공 또는 시간 초과 후 원래 BT 행동으로 복귀
    void FinishMovementRecovery(
        UBlackboardComponent& BlackboardComponent,
        const FVector& CurrentLocation
    );

private:

    // Blackboard에서 사용하는 고정 키 이름
    static const FName IsMovementRecoveringKeyName;
    static const FName MovementRecoveryLocationKeyName;

    // 한 번 검사할 때 이 거리보다 적게 움직였다면
    // 제자리에 머문 것으로 판정
    UPROPERTY(
        EditAnywhere,
        Category = "Monster|AI|Movement Recovery",
        meta = (ClampMin = "0.0")
    )
    float MinimumMovementDistance = 5.0f;

    // 이 시간 동안 계속 움직이지 못하면 복구 시작
    UPROPERTY(
        EditAnywhere,
        Category = "Monster|AI|Movement Recovery",
        meta = (ClampMin = "0.1")
    )
    float StuckDetectionDuration = 1.5f;

    // 현재 위치 주변에서 복구 지점을 찾을 범위
    UPROPERTY(
        EditAnywhere,
        Category = "Monster|AI|Movement Recovery",
        meta = (ClampMin = "50.0")
    )
    float RecoverySearchRadius = 250.0f;

    // 현재 위치와 너무 가까운 지점은 복구 위치에서 제외
    UPROPERTY(
        EditAnywhere,
        Category = "Monster|AI|Movement Recovery",
        meta = (ClampMin = "0.0")
    )
    float MinimumRecoveryDistance = 80.0f;

    // 이 거리 안까지 이동하면 복구에 성공했다고 판정
    UPROPERTY(
        EditAnywhere,
        Category = "Monster|AI|Movement Recovery",
        meta = (ClampMin = "1.0")
    )
    float RecoveryCompletionRadius = 100.0f;

    // 복구 이동이 끝나지 않더라도 이 시간이 지나면
    // 복구 상태를 해제하여 무한 정지를 방지
    UPROPERTY(
        EditAnywhere,
        Category = "Monster|AI|Movement Recovery",
        meta = (ClampMin = "0.1")
    )
    float RecoveryTimeout = 3.0f;

    // 적합한 NavMesh 복구 지점을 찾기 위한 최대 시도 횟수
    UPROPERTY(
        EditAnywhere,
        Category = "Monster|AI|Movement Recovery",
        meta = (ClampMin = "1")
    )
    int32 RecoveryPointSearchAttempts = 8;

    // 마지막 검사에서 확인한 몬스터 위치
    FVector LastObservedLocation = FVector::ZeroVector;

    // 이동하지 못한 누적 시간
    float StationaryElapsedTime = 0.0f;

    // 현재 복구를 진행한 시간
    float RecoveryElapsedTime = 0.0f;

    // 첫 위치가 저장됐는지 구분
    bool bHasLastObservedLocation = false;
};
