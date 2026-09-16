

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_BaruMovementRecovery.generated.h"


class AAIController;
class ACharacter;
class UBlackboardComponent;
class UNavigationPath;

/**
 * 이동 요청 중 제자리걸음을 감지하고, 충돌을 검사한 주변 지점으로 빠져나온다.
 * 공통 상위 Selector에 붙여야 복구 분기에서도 서비스가 계속 실행된다.
 * BT의 복구 Selector에는 Move To와 실패 시 사용할 Wait가 필요하다.
 * 역할 분담:
 * - 이 서비스: 막힘 감지, 복구 목적지 선정, 성공/실패 판단.
 * - Blackboard: 서비스와 BT 사이에서 Bool과 목적지 좌표를 전달.
 * - BT Move To: 전달받은 목적지까지 실제 이동 요청 실행.
 * 거리 단위는 cm, 시간 단위는 초다.
 */
UCLASS()
class BARUGAME_API UBTService_BaruMovementRecovery : public UBTService
{
    GENERATED_BODY()

public:
    
    UBTService_BaruMovementRecovery();

protected:
    /**
     * 서비스가 활성화된 동안 설정한 주기로 호출되는 진입점.
     * OwnerComp: 이 서비스를 실행하는 Behavior Tree 컴포넌트.
     * NodeMemory: 엔진이 제공하는 노드 메모리. 이 구현은 인스턴스 멤버를 사용한다.
     * DeltaSeconds: 이번 갱신에 전달된 경과 시간. 누적 타이머에 더한다.
     * override는 부모의 가상 함수를 재정의한다는 뜻이다.
     * void 함수이므로 중간 종료에는 return;을 사용하고 값을 반환하지 않는다.
     */
    virtual void TickNode(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory,
        float DeltaSeconds) override;

private:
    /**
     * 옆/뒤 방향부터 후보를 만들어 NavMesh와 실제 충돌을 검사한다.
     * true: OutLocation에 사용할 목적지를 기록했다.
     * false: 이번 검색에서 적합한 후보를 찾지 못했다.
     *
     * const ACharacter&: 캐릭터를 복사하지 않고 읽기 전용 참조로 받는다.
     * FVector& OutLocation: 호출자가 넘긴 변수에 결과를 써 주는 출력 인자다.
     * 함수 뒤의 const: 이 함수에서 서비스의 일반 멤버 상태를 변경하지 않는다.
     */
    bool FindRecoveryLocation(
        const ACharacter& Character, FVector& OutLocation) const;

    // NavMesh 경로에 실제 캡슐이 통과할 공간도 있는지 검사한다.
    // true는 검사 시점에 막는 충돌이 없다는 뜻이며, 미래의 장애물 이동 보장은 아니다.
    bool IsRecoveryPathClear(
        const ACharacter& Character, const UNavigationPath& Path) const;

    // 기준점 주변에 머문 시간을 누적하고, 감지 시간을 넘으면 true를 반환한다.
    // 매번 이전 Tick과 비교하는 대신 동일한 기준점에서 얼마나 벗어났는지 본다.
    bool HasStayedNearAnchor(const FVector& Location, float DeltaSeconds);

    // 현재 위치를 새 기준점으로 삼고 정지 시간만 초기화한다.
    // 복구 시도 횟수나 전체 복구 이동 시간까지 지우는 함수는 아니다.
    void ResetMovementObservation(const FVector& Location);

    // 후보 검색 성공 시 목적지를 Blackboard에 기록한다.
    // 실패 시 ScheduleRecoveryRetry를 호출한다. 실제 이동은 BT가 맡는다.
    void TryStartRecoveryMove(
        AAIController& Controller,
        UBlackboardComponent& Blackboard,
        const ACharacter& Character);

    // 실패 시 복구 Bool을 유지하고, 이동을 멈춘 상태에서 다음 후보를 기다린다.
    // Reason은 로그에 남길 이유이며 TEXT("Timeout") 같은 문자열을 전달한다.
    void ScheduleRecoveryRetry(
        AAIController& Controller,
        UBlackboardComponent& Blackboard,
        const TCHAR* Reason);

    // 실제로 목적지에 도착한 경우에만 일반 행동으로 복귀한다.
    void FinishMovementRecovery(
        AAIController& Controller,
        UBlackboardComponent& Blackboard,
        const FVector& CurrentLocation);
    
    // FName은 Blackboard 키를 찾는 이름 식별자다. 에셋의 이름과 정확히 같아야 한다.
    // static const이므로 모든 서비스 인스턴스가 같은 키 이름을 공유한다.
    // 실제 값은 CPP의 클래스명::키이름 정의에서 지정한다.
    static const FName IsMovementRecoveringKeyName;
    static const FName MovementRecoveryLocationKeyName;

    // UPROPERTY: 언리얼이 이 설정을 인식하도록 한다.
    // EditAnywhere: 에디터에서 수정 가능. Category: 디테일 패널의 분류.
    // ClampMin/ClampMax: 에디터 입력 범위. 실행 중에도 FMath로 별도 보정한다.
    // 기존 BT 에셋에 저장한 값은 C++ 초기값 변경 후에도 남아 있을 수 있다.

    // 매 Tick 이동량이 아니라 기준 위치에서 이만큼 벗어났는지 검사한다.
    // 5~10cm를 왕복하는 흔들림은 이동 성공으로 누적하지 않는다.
    UPROPERTY(EditAnywhere, Category = "Monster|AI|Movement Recovery",
        meta = (ClampMin = "20.0"))
    float MinimumMovementDistance = 40.0f;

    // 기준점에서 충분히 벗어나지 못한 시간이 1.5초가 되면 막힘으로 판단한다.
    // 길게 설정하면 감지는 늦어지고, 너무 짧으면 잠깐 멈춘 것도 복구할 수 있다.
    UPROPERTY(EditAnywhere, Category = "Monster|AI|Movement Recovery",
        meta = (ClampMin = "0.1"))
    float StuckDetectionDuration = 1.5f;

    // 후보 탐색의 기본 거리. 실패 횟수와 거리 배율에 따라 실제 검사 거리는 달라진다.
    // 넓히면 멀리 돌아갈 후보를 찾을 수 있지만 국소 복구 범위도 커진다.
    UPROPERTY(EditAnywhere, Category = "Monster|AI|Movement Recovery",
        meta = (ClampMin = "100.0", ClampMax = "600.0"))
    float RecoverySearchRadius = 320.0f;

    // 완료 반경보다 충분히 멀어야 제자리에서 복구가 끝나지 않는다.
    UPROPERTY(EditAnywhere, Category = "Monster|AI|Movement Recovery",
        meta = (ClampMin = "50.0"))
    float MinimumRecoveryDistance = 180.0f;

    // BT Move To의 Acceptable Radius도 30으로 설정한다.
    // Reach Test Includes Agent/Goal Radius는 모두 끈다.
    UPROPERTY(EditAnywhere, Category = "Monster|AI|Movement Recovery",
        meta = (ClampMin = "1.0", ClampMax = "100.0"))
    float RecoveryCompletionRadius = 30.0f;

    // 한 후보로 이동할 수 있는 최대 시간. 시간 초과는 성공이 아니다.
    UPROPERTY(EditAnywhere, Category = "Monster|AI|Movement Recovery",
        meta = (ClampMin = "1.5"))
    float RecoveryTimeout = 3.0f;

    // 한 번의 후보 검색에서 수행할 최대 NavMesh 투영 횟수.
    UPROPERTY(EditAnywhere, Category = "Monster|AI|Movement Recovery",
        meta = (ClampMin = "1", ClampMax = "24"))
    int32 RecoveryPointSearchAttempts = 16;

    // 한 후보가 실패한 뒤 다음 후보를 찾기까지 기다릴 기본 시간.
    // 개체별 지연 0~0.3초가 추가되므로 모두 동시에 재출발하지 않는다.
    UPROPERTY(EditAnywhere, Category = "Monster|AI|Movement Recovery",
        meta = (ClampMin = "0.5"))
    float RecoveryRetryDelay = 0.7f;

    // 연속 실패하면 더 오래 기다리고 후보 기록을 비운 뒤 다시 검사한다.
    UPROPERTY(EditAnywhere, Category = "Monster|AI|Movement Recovery",
        meta = (ClampMin = "1.0"))
    float RecoveryBlockedWait = 2.0f;

    // 이 횟수만큼 연속 실패하면 짧은 재시도 대신 RecoveryBlockedWait를 적용한다.
    // 전체 복구 종료 횟수가 아니다. 긴 대기 후에도 복구 검색은 계속한다.
    UPROPERTY(EditAnywhere, Category = "Monster|AI|Movement Recovery",
        meta = (ClampMin = "1", ClampMax = "8"))
    int32 MaximumConsecutiveFailures = 3;

    // 아래는 에디터 설정이 아닌, 각 몬스터의 실행 중 관찰 상태다.

    // 제자리걸음 비교 기준 위치. 작은 흔들림마다 갱신하면 막힘을 놓칠 수 있다.
    FVector ObservationAnchor = FVector::ZeroVector;

    // 막히기 직전에 가려던 수평 방향. 재시도 때도 이 방향을 기준으로 옆/뒤를 찾는다.
    FVector BlockedMoveDirection = FVector::ForwardVector;

    // 지금 이동을 시도하는 복구 목적지. 도착 판정과 실패 기록에 사용한다.
    FVector ActiveRecoveryLocation = FVector::ZeroVector;

    // 이번 목적지를 지정했을 때의 위치. 실제로 제자리에서 벗어났는지 확인한다.
    FVector RecoveryStartLocation = FVector::ZeroVector;

    // BT가 상위 노드를 다시 검색해도 복구 상태를 지우지 않는다.
    // 같은 서비스 인스턴스가 다른 Pawn을 맡을 때만 초기화한다.
    // TWeakObjectPtr은 객체를 살려 두지 않는 약한 참조다.
    // 객체가 파괴되면 Get()으로 유효한 객체를 얻을 수 없게 된다.
    TWeakObjectPtr<ACharacter> ObservedCharacter;

    // 최근 실패한 후보의 가까운 이웃도 잠시 제외한다.
    TArray<FVector> FailedRecoveryLocations;

    // 누적 타이머: 기준점 근처에 머무는 동안 더하고, 벗어나면 0으로 만든다.
    float StationaryElapsedTime = 0.0f;

    // 이번 복구 목적지를 지정한 뒤의 시간. 목적지가 바뀌면 다시 0부터 센다.
    float RecoveryElapsedTime = 0.0f;

    // 카운트다운 타이머: 재시도 대기 중 빼고, 0 이하가 되면 다시 후보를 찾는다.
    float RetryDelayRemaining = 0.0f;

    // 도착 전 실패 및 후보 검색 실패 횟수. 긴 대기 후 초기화한다.
    int32 ConsecutiveFailures = 0;

    // FVector::ZeroVector는 실제 월드 원점일 수도 있으므로 별도 Bool로 초기화 여부 구분.
    bool bHasObservationAnchor = false;

    // 복구 Bool이 true인 동안: true면 재시도 대기, false면 목적지로 이동하는 단계.
    // 복구 중인지 자체는 Blackboard의 IsMovementRecovering 값으로 확인한다.
    bool bWaitingForRetry = false;

    // 긴 대기가 끝날 때 실패 후보를 비워야 한다는 예약 표시다.
    // 즉시 지우지 않아 대기 전에 같은 후보를 다시 고르는 일을 막는다.
    bool bResetCandidatesAfterWait = false;
};
