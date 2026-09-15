

#include "BTTask_BaruFinishAmbush.h"

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Monster/AI/BaruMonsterAIController.h"

UBTTask_BaruFinishAmbush::UBTTask_BaruFinishAmbush()
{
	// Behavior Tree 편집기에 표시될 이름
	NodeName = TEXT("Finish Ambush");

	// 한 번 실행한 뒤 바로 성공 또는 실패를 반환하므로
	// Tick은 사용하지 않음
	bNotifyTick = false;
}

EBTNodeResult::Type UBTTask_BaruFinishAmbush::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory
)
{
	// 이 Behavior Tree를 실행 중인 몬스터 AIController 확인
	ABaruMonsterAIController* MonsterAI =
		Cast<ABaruMonsterAIController>(OwnerComp.GetAIOwner());

	// 다른 종류의 AIController이거나 이미 제거됐다면
	// 매복 명령을 정리할 수 없으므로 실패
	if (!IsValid(MonsterAI))
	{
		return EBTNodeResult::Failed;
	}

	/*
	 * ClearDirectorCommand 내부에서 다음 값들이 정리됨:
	 *
	 * - HasAmbushOrder
	 * - AmbushTargetActor
	 * - AmbushLocation
	 * - IsAmbushReady
	 * - ShouldSpringAmbush
	 *
	 * 이후 몬스터는 디렉터 명령이 없는
	 * 일반 AI 판단 상태로 돌아감
	 */
	MonsterAI->ClearDirectorCommand();

	// 매복 종료 처리가 완료됐으므로 성공 반환
	return EBTNodeResult::Succeeded;
}
