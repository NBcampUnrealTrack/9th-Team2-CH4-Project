


#include "BTTask_BaruCompleteEncirclement.h"

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Monster/AI/BaruMonsterAIController.h"


UBTTask_BaruCompleteEncirclement::
	UBTTask_BaruCompleteEncirclement()
{
	// Behavior Tree 에디터에 표시될 태스크 이름
	NodeName = TEXT("Complete Encirclement");

	// 태스크 내부에 몬스터별 실행 상태를 저장하지 않으므로
	// 모든 AI가 하나의 노드 인스턴스를 공유해도 안전
	bCreateNodeInstance = false;
}

EBTNodeResult::Type
UBTTask_BaruCompleteEncirclement::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory
)
{
	// 이 Behavior Tree를 실행하는 AIController 가져오기
	ABaruMonsterAIController* MonsterController =
		Cast<ABaruMonsterAIController>(
			OwnerComp.GetAIOwner()
		);

	// 포위 명령 판단과 상태 변경은 서버에서만 수행
	if (!IsValid(MonsterController) ||
		!MonsterController->HasAuthority())
	{
		return EBTNodeResult::Failed;
	}

	/*
	 * 우회 지점과 최종 차단 위치에 도착했음을 전달
	 *
	 * AIController는 Encircle 명령을 Hold 상태로 전환하고
	 * 포위 관련 Blackboard 값을 갱신한다.
	 */
	MonsterController->
		CompleteDirectorEncirclementCommand();

	// 완료 전달에 성공했으므로 BT Sequence 계속 진행
	return EBTNodeResult::Succeeded;
}
