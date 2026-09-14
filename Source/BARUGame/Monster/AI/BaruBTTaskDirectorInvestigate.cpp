


#include "BaruBTTaskDirectorInvestigate.h"

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AITypes.h"

#include "Monster/AI/BaruMonsterAIController.h"
#include "Monster/Characters/BaruMonsterCharacter.h"
#include "Monster/Data/BaruMonsterDataAsset.h"
#include "Monster/Navigation/BaruNavigationQueryFilterSameFloor.h"

#include "Interfaces/CombatInterface.h"
#include "BaruLog.h"

UBaruBTTaskDirectorInvestigate::UBaruBTTaskDirectorInvestigate()
{
    NodeName = TEXT("Director Investigate");

    // 실행 정보를 몬스터별로 분리
    bCreateNodeInstance = true;

    // 종료 시 OnTaskFinished 호출
    bNotifyTaskFinished = true;

    // 기본 목적지 키
    BlackboardKey.SelectedKeyName =
        TEXT("DirectorTargetLocation");
}

EBTNodeResult::Type UBaruBTTaskDirectorInvestigate::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory
)
{
    bHasExecutingCommand = false;

    ABaruMonsterAIController* MonsterController =
        Cast<ABaruMonsterAIController>(OwnerComp.GetAIOwner());

    if (!IsValid(MonsterController) ||
        !MonsterController->HasAuthority())
    {
        return EBTNodeResult::Failed;
    }

    ABaruMonsterCharacter* MonsterCharacter =
        Cast<ABaruMonsterCharacter>(MonsterController->GetPawn());

    if (!IsValid(MonsterCharacter) ||
        ICombatInterface::Execute_IsDead(MonsterCharacter))
    {
        return EBTNodeResult::Failed;
    }

    if (MonsterController->GetDirectorCommand() !=
        EBaruMonsterDirectorCommand::Investigate)
    {
        return EBTNodeResult::Failed;
    }

    // 지금 실행하는 명령을 기억
    ExecutingCommandRevision =
        MonsterController->GetDirectorCommandRevision();

    bHasExecutingCommand = true;

    // 경로 요청과 도착 대기는 부모 MoveTo Task가 담당
    return Super::ExecuteTask(OwnerComp, NodeMemory);
}

UAITask_MoveTo* UBaruBTTaskDirectorInvestigate::PrepareMoveTask(
    UBehaviorTreeComponent& OwnerComp,
    UAITask_MoveTo* ExistingTask,
    FAIMoveRequest& MoveRequest
)
{
    ABaruMonsterAIController* MonsterController =
        Cast<ABaruMonsterAIController>(OwnerComp.GetAIOwner());

    ABaruMonsterCharacter* MonsterCharacter =
        IsValid(MonsterController)
        ? Cast<ABaruMonsterCharacter>(MonsterController->GetPawn())
        : nullptr;

    if (IsValid(MonsterCharacter))
    {
        // 맵 배치 몬스터의 일반 조사 이동은 층 연결 구간 차단
        // 자유 층 이동 개체는 부모가 설정한 필터를 그대로 사용
        if (!MonsterCharacter->CanTraverseFloorsWhileIdle())
        {
            MoveRequest.SetNavigationFilter(
                UBaruNavigationQueryFilterSameFloor::StaticClass()
            );
        }

        if (const UBaruMonsterDataAsset* MonsterData =
            MonsterCharacter->GetMonsterDataAsset())
        {
            MoveRequest.SetAcceptanceRadius(
                FMath::Max(0.0f, MonsterData->MoveAcceptanceRadius)
            );
        }
    }

    // 목적지까지 갈 수 없는 부분 경로는 사용하지 않음
    // 계단 앞까지만 이동하고 도착했다고 처리되는 것을 방지
    MoveRequest.SetAllowPartialPath(false);

    return Super::PrepareMoveTask(
        OwnerComp,
        ExistingTask,
        MoveRequest
    );
}

void UBaruBTTaskDirectorInvestigate::OnTaskFinished(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory,
    EBTNodeResult::Type TaskResult
)
{
    // 콜백 중 상태가 바뀌어도 이번 실행 정보를 유지
    const bool bHadCommand = bHasExecutingCommand;
    const uint32 FinishedRevision = ExecutingCommandRevision;

    bHasExecutingCommand = false;

    // 부모의 이동 Task 정리를 먼저 실행
    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);

    // 추적·그로기 등으로 중단됐다면 조사 명령은 유지
    if (!bHadCommand || TaskResult == EBTNodeResult::Aborted)
    {
        return;
    }

    ABaruMonsterAIController* MonsterController =
        Cast<ABaruMonsterAIController>(OwnerComp.GetAIOwner());

    if (!IsValid(MonsterController) ||
        !MonsterController->HasAuthority())
    {
        return;
    }

    // 실행 중 다른 명령이 들어왔다면 새 명령은 지우지 않음
    if (MonsterController->GetDirectorCommandRevision() !=
        FinishedRevision ||
        MonsterController->GetDirectorCommand() !=
        EBaruMonsterDirectorCommand::Investigate)
    {
        return;
    }

    BARU_NET_LOG(
        MonsterController,
        LogBaruAI,
        Log,
        TEXT("Director investigation finished: %s"),
        TaskResult == EBTNodeResult::Succeeded
            ? TEXT("Reached destination")
            : TEXT("Movement failed")
    );

    // 도착하거나 경로 이동에 실패하면 명령을 정리
    // 실패한 목적지를 무한히 재시도하지 않도록 함
    MonsterController->ClearDirectorCommand();
}