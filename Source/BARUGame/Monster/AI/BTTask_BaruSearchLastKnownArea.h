

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_BaruSearchLastKnownArea.generated.h"

class AAIController;
class ABaruMonsterAIController;
class UBehaviorTreeComponent;

/**
 * 플레이어를 놓친 몬스터가 마지막 목격 위치로 이동한 뒤
 * 주변의 이동 가능한 지점을 순차적으로 수색하는 태스크
 *
 * 처리 순서:
 * 1. 마지막 목격 위치로 이동
 * 2. 도착 후 수색 시간 측정 시작
 * 3. 주변의 이동 가능한 위치를 최대 지정 횟수만큼 확인
 * 4. 플레이어를 다시 발견하거나 수색이 끝나면 태스크 종료
 */
UCLASS()
class BARUGAME_API UBTTask_BaruSearchLastKnownArea
    : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_BaruSearchLastKnownArea();

protected:
    virtual EBTNodeResult::Type ExecuteTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory
    ) override;

    virtual void TickTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory,
        float DeltaSeconds
    ) override;

    virtual EBTNodeResult::Type AbortTask(
        UBehaviorTreeComponent& OwnerComp,
        uint8* NodeMemory
    ) override;

private:
    // 태스크가 현재 수행 중인 단계
    enum class ESearchPhase : uint8
    {
        MovingToLastKnownLocation,
        SearchingArea
    };

    // 마지막 목격 위치 주변을 탐색할 반경
    UPROPERTY(
        EditAnywhere,
        Category = "Search",
        meta = (ClampMin = "100.0", Units = "cm")
    )
    float SearchRadius = 500.0f;

    // 한 번의 수색에서 확인할 최대 위치 수
    UPROPERTY(
        EditAnywhere,
        Category = "Search",
        meta = (ClampMin = "1", ClampMax = "10")
    )
    int32 MaximumSearchPointCount = 3;

    // 각 수색 위치에 도착한 뒤 주변을 살피는 시간
    UPROPERTY(
        EditAnywhere,
        Category = "Search",
        meta = (ClampMin = "0.0", Units = "s")
    )
    float WaitAtSearchPointDuration = 0.75f;

    // 이동 완료로 인정할 거리
    UPROPERTY(
        EditAnywhere,
        Category = "Search",
        meta = (ClampMin = "10.0", Units = "cm")
    )
    float MoveAcceptanceRadius = 80.0f;

    // 마지막 목격 위치로 이동하다 영구적으로 막히는 것을 방지하는
    // 안전 제한 시간이며 실제 주변 수색 시간과는 별개
    UPROPERTY(
        EditAnywhere,
        Category = "Search",
        meta = (ClampMin = "1.0", Units = "s")
    )
    float MaximumApproachDuration = 10.0f;

    // 주변에서 새로운 이동 가능 지점을 찾아 이동 시작
    bool TryMoveToNextSearchPoint(
        AAIController* AIController
    );

    // 마지막 위치 도착 후 실제 주변 수색 단계 시작
    void BeginAreaSearch();

    // 수색 결과를 정리하고 Behavior Tree 태스크 종료
    void FinishSearch(
        UBehaviorTreeComponent& OwnerComp,
        ABaruMonsterAIController* MonsterAI
    );

private:
    ESearchPhase CurrentPhase =
        ESearchPhase::MovingToLastKnownLocation;

    FVector SearchOrigin = FVector::ZeroVector;

    float ApproachElapsedTime = 0.0f;
    float SearchElapsedTime = 0.0f;
    float CurrentPointWaitTime = 0.0f;
    float MaximumSearchDuration = 0.0f;

    int32 CompletedSearchPointCount = 0;

    bool bMoveRequestActive = false;
    bool bWaitingAtSearchPoint = false;
    bool bTaskFinishing = false;
};
