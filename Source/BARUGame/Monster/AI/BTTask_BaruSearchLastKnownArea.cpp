


#include "BTTask_BaruSearchLastKnownArea.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"

#include "Monster/AI/BaruMonsterAIController.h"
#include "Monster/Characters/BaruMonsterCharacter.h"
#include "Monster/Data/BaruMonsterDataAsset.h"

#include "BaruLog.h"

UBTTask_BaruSearchLastKnownArea::
UBTTask_BaruSearchLastKnownArea()
{
    NodeName = TEXT("Search Last Known Area");

    // 이동 완료와 수색 시간을 매 프레임 확인
    bNotifyTick = true;

    /*
     * 수색 위치와 경과시간은 몬스터마다 달라야 함
     * Behavior Tree 노드를 공유하지 않고
     * AI마다 별도의 태스크 인스턴스를 생성
     */
    bCreateNodeInstance = true;
}

EBTNodeResult::Type
UBTTask_BaruSearchLastKnownArea::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory
)
{
    ABaruMonsterAIController* MonsterAI =
        Cast<ABaruMonsterAIController>(
            OwnerComp.GetAIOwner()
        );

    if (!IsValid(MonsterAI) ||
        !MonsterAI->HasAuthority() ||
        !MonsterAI->HasLastKnownTargetLocation())
    {
        return EBTNodeResult::Failed;
    }

    ABaruMonsterCharacter* MonsterCharacter =
        Cast<ABaruMonsterCharacter>(
            MonsterAI->GetPawn()
        );

    if (!IsValid(MonsterCharacter))
    {
        return EBTNodeResult::Failed;
    }

    const UBaruMonsterDataAsset* MonsterData =
        MonsterCharacter->GetMonsterDataAsset();

    if (!IsValid(MonsterData))
    {
        return EBTNodeResult::Failed;
    }

    // 이전 실행에서 남아 있을 수 있는 상태 초기화
    CurrentPhase =
        ESearchPhase::MovingToLastKnownLocation;

    SearchOrigin =
        MonsterAI->GetLastKnownTargetLocation();

    ApproachElapsedTime = 0.0f;
    SearchElapsedTime = 0.0f;
    CurrentPointWaitTime = 0.0f;

    CompletedSearchPointCount = 0;

    bMoveRequestActive = false;
    bWaitingAtSearchPoint = false;
    bTaskFinishing = false;

    /*
     * SightMemoryDuration은 플레이어를 놓친 순간부터가 아니라
     * 마지막 위치에 도착한 뒤 주변을 수색할 시간으로 사용
     */
    MaximumSearchDuration =
        FMath::Max(
            0.1f,
            MonsterData->SightMemoryDuration
        );

    // 이전 Behavior Tree 이동 요청이 남아 있다면 정리
    MonsterAI->StopMovement();

    /*
     * 먼저 마지막 목격 위치로 이동
     * 목적지는 NavMesh 위로 투영하며 부분 경로도 허용
     */
    const EPathFollowingRequestResult::Type MoveResult =
        MonsterAI->MoveToLocation(
            SearchOrigin,
            MoveAcceptanceRadius,
            true,
            true,
            true,
            false,
            nullptr,
            true
        );

    if (MoveResult ==
        EPathFollowingRequestResult::RequestSuccessful)
    {
        bMoveRequestActive = true;
    }
    else if (MoveResult ==
             EPathFollowingRequestResult::AlreadyAtGoal)
    {
        // 이미 마지막 목격 위치 근처라면 바로 주변 수색 시작
        BeginAreaSearch();
    }
    else
    {
        /*
         * 마지막 위치로 직접 가는 경로를 만들지 못했더라도
         * 태스크를 즉시 실패시켜 반복 실행하지 않음
         * 현재 위치에서 접근 가능한 주변 지점을 찾아 수색
         */
        BeginAreaSearch();

        BARU_NET_LOG(
            MonsterAI,
            LogBaruAI,
            Warning,
            TEXT(
                "Failed to move directly to last known location. "
                "Starting nearby search instead: %s"
            ),
            *SearchOrigin.ToString()
        );
    }

    return EBTNodeResult::InProgress;
}

void UBTTask_BaruSearchLastKnownArea::TickTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory,
    float DeltaSeconds
)
{
    if (bTaskFinishing)
    {
        return;
    }

    ABaruMonsterAIController* MonsterAI =
        Cast<ABaruMonsterAIController>(
            OwnerComp.GetAIOwner()
        );

    if (!IsValid(MonsterAI))
    {
        FinishLatentTask(
            OwnerComp,
            EBTNodeResult::Failed
        );
        return;
    }

    /*
     * 플레이어를 다시 발견하면 AIController가
     * 마지막 목격 위치를 먼저 삭제함
     * 이 경우 수색 이동을 중단하고 추적 분기로 넘김
     */
    if (!MonsterAI->HasLastKnownTargetLocation() ||
        IsValid(MonsterAI->GetCurrentTarget()))
    {
        MonsterAI->StopMovement();

        FinishLatentTask(
            OwnerComp,
            EBTNodeResult::Succeeded
        );
        return;
    }

    // ---------------------------------------------------------
    // 마지막 목격 위치로 접근하는 단계
    // 이 단계에서는 실제 주변 수색 시간을 감소시키지 않음
    // ---------------------------------------------------------
    if (CurrentPhase ==
        ESearchPhase::MovingToLastKnownLocation)
    {
        ApproachElapsedTime += DeltaSeconds;

        const EPathFollowingStatus::Type MoveStatus =
            MonsterAI->GetMoveStatus();

        if (MoveStatus == EPathFollowingStatus::Moving ||
            MoveStatus == EPathFollowingStatus::Waiting)
        {
            /*
             * 큰 장애물이나 잘못된 부분 경로 때문에
             * 이동이 영원히 유지되는 상황을 방지
             */
            if (ApproachElapsedTime <
                MaximumApproachDuration)
            {
                return;
            }

            MonsterAI->StopMovement();

            BARU_NET_LOG(
                MonsterAI,
                LogBaruAI,
                Warning,
                TEXT(
                    "Last known location approach timed out. "
                    "Starting nearby search: %s"
                ),
                *SearchOrigin.ToString()
            );
        }

        /*
         * 정상 도착, 부분 경로 종료, 이동 실패 모두 여기서
         * 주변 수색 단계로 전환
         */
        BeginAreaSearch();
    }

    // ---------------------------------------------------------
    // 마지막 목격 위치 주변을 실제로 수색하는 단계
    // ---------------------------------------------------------
    SearchElapsedTime += DeltaSeconds;

    // DataAsset의 수색 시간이 끝났거나 지정 횟수를 모두 확인
    if (SearchElapsedTime >= MaximumSearchDuration ||
        CompletedSearchPointCount >=
            MaximumSearchPointCount)
    {
        FinishSearch(
            OwnerComp,
            MonsterAI
        );
        return;
    }

    // 현재 수색 지점으로 이동 중
    if (bMoveRequestActive)
    {
        const EPathFollowingStatus::Type MoveStatus =
            MonsterAI->GetMoveStatus();

        if (MoveStatus == EPathFollowingStatus::Moving ||
            MoveStatus == EPathFollowingStatus::Waiting)
        {
            return;
        }

        /*
         * 정상 도착 또는 이동 중 경로 단절 모두
         * 해당 지점을 한 번 확인한 것으로 계산
         */
        bMoveRequestActive = false;
        bWaitingAtSearchPoint = true;
        CurrentPointWaitTime = 0.0f;

        ++CompletedSearchPointCount;

        return;
    }

    // 지점에 도착한 뒤 잠시 멈춰 주변을 확인
    if (bWaitingAtSearchPoint)
    {
        CurrentPointWaitTime += DeltaSeconds;

        if (CurrentPointWaitTime <
            WaitAtSearchPointDuration)
        {
            return;
        }

        bWaitingAtSearchPoint = false;
        CurrentPointWaitTime = 0.0f;
    }

    if (CompletedSearchPointCount >=
        MaximumSearchPointCount)
    {
        FinishSearch(
            OwnerComp,
            MonsterAI
        );
        return;
    }

    // 다음 이동 가능한 수색 위치 탐색
    TryMoveToNextSearchPoint(MonsterAI);
}

EBTNodeResult::Type
UBTTask_BaruSearchLastKnownArea::AbortTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory
)
{
    // 추적 등 더 높은 우선순위 행동으로 전환될 때
    // 현재 진행 중이던 수색 이동만 취소
    if (AAIController* AIController =
        OwnerComp.GetAIOwner())
    {
        AIController->StopMovement();
    }

    bMoveRequestActive = false;
    bWaitingAtSearchPoint = false;
    bTaskFinishing = false;

    return EBTNodeResult::Aborted;
}

bool
UBTTask_BaruSearchLastKnownArea::
TryMoveToNextSearchPoint(
    AAIController* AIController
)
{
    if (!IsValid(AIController) ||
        !IsValid(AIController->GetPawn()))
    {
        return false;
    }

    UWorld* World = AIController->GetWorld();

    if (!IsValid(World))
    {
        return false;
    }

    UNavigationSystemV1* NavigationSystem =
        FNavigationSystem::GetCurrent<
            UNavigationSystemV1
        >(World);

    if (!IsValid(NavigationSystem))
    {
        return false;
    }

    constexpr int32 MaximumLocationAttempts = 6;

    for (int32 AttemptIndex = 0;
         AttemptIndex < MaximumLocationAttempts;
         ++AttemptIndex)
    {
        FNavLocation SearchPoint;

        /*
         * 마지막 목격 위치 주변에서
         * NavMesh로 실제 도달 가능한 무작위 위치 탐색
         */
        if (!NavigationSystem->
            GetRandomReachablePointInRadius(
                SearchOrigin,
                SearchRadius,
                SearchPoint
            ))
        {
            continue;
        }

        const FVector CurrentLocation =
            AIController->GetPawn()->
                GetActorLocation();

        /*
         * 현재 위치와 너무 가까운 점은 제외
         * 제자리 수색만 반복되는 현상을 줄임
         */
        const float MinimumMoveDistance =
            FMath::Max(
                MoveAcceptanceRadius * 2.0f,
                150.0f
            );

        if (FVector::DistSquared2D(
                CurrentLocation,
                SearchPoint.Location
            ) <
            FMath::Square(MinimumMoveDistance))
        {
            continue;
        }

        const EPathFollowingRequestResult::Type
            MoveResult =
                AIController->MoveToLocation(
                    SearchPoint.Location,
                    MoveAcceptanceRadius,
                    true,
                    true,
                    true,
                    false,
                    nullptr,
                    false
                );

        if (MoveResult ==
            EPathFollowingRequestResult::Failed)
        {
            continue;
        }

        if (MoveResult ==
            EPathFollowingRequestResult::
                AlreadyAtGoal)
        {
            ++CompletedSearchPointCount;

            bWaitingAtSearchPoint = true;
            CurrentPointWaitTime = 0.0f;
        }
        else
        {
            bMoveRequestActive = true;
        }

        return true;
    }

    /*
     * 이번에는 유효한 지점을 찾지 못했음
     * 매 프레임 즉시 재시도하지 않도록 잠시 대기한 뒤 다시 시도
     */
    ++CompletedSearchPointCount;

    bWaitingAtSearchPoint = true;
    CurrentPointWaitTime = 0.0f;

    return false;
}

void
UBTTask_BaruSearchLastKnownArea::BeginAreaSearch()
{
    CurrentPhase = ESearchPhase::SearchingArea;

    // 실제 수색 시간은 이 시점부터 시작
    SearchElapsedTime = 0.0f;
    CurrentPointWaitTime = 0.0f;

    bMoveRequestActive = false;
    bWaitingAtSearchPoint = false;
}

void
UBTTask_BaruSearchLastKnownArea::FinishSearch(
    UBehaviorTreeComponent& OwnerComp,
    ABaruMonsterAIController* MonsterAI
)
{
    if (bTaskFinishing)
    {
        return;
    }

    bTaskFinishing = true;

    if (IsValid(MonsterAI))
    {
        MonsterAI->StopMovement();

        /*
         * 수색 완료 사실을 AIController에 전달
         * 마지막 위치와 Blackboard 값은 Controller가 정리
         */
        MonsterAI->CompleteLastKnownTargetSearch();
    }

    FinishLatentTask(
        OwnerComp,
        EBTNodeResult::Succeeded
    );
}
