

#include "BaruBTTask_SetMonsterMoveSpeed.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Monster/Components/BaruMonsterNavigationComponent.h"

#include "BaruLog.h"

UBaruBTTask_SetMonsterMoveSpeed::UBaruBTTask_SetMonsterMoveSpeed()
{
	NodeName = TEXT("Set Monster Move Speed");
}

EBTNodeResult::Type
UBaruBTTask_SetMonsterMoveSpeed::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory
)
{
	// 이 BT를 실행 중인 AIController를 가져옴
	AAIController* OwnerController =
		OwnerComp.GetAIOwner();

	// AI 판단과 속도 변경은 서버에서만 실행
	if (!IsValid(OwnerController) ||
		!OwnerController->HasAuthority())
	{
		return EBTNodeResult::Failed;
	}

	// AIController가 소유한 몬스터 이동 컴포넌트를 찾음
	UBaruMonsterNavigationComponent* NavigationComponent =
		OwnerController->FindComponentByClass<
			UBaruMonsterNavigationComponent
		>();

	if (!IsValid(NavigationComponent))
	{
		BARU_NET_LOG(
			OwnerController,
			LogBaruAI,
			Error,
			TEXT("Monster Navigation Component is invalid.")
		);

		return EBTNodeResult::Failed;
	}

	// DataAsset에서 적용받은 배회 또는 추적 속도를 선택
	const float DesiredMoveSpeed =
		MoveSpeedMode == EBaruMonsterMoveSpeedMode::Chase
		? NavigationComponent->GetChaseSpeed()
		: NavigationComponent->GetPatrolSpeed();

	// GAS CoreAttributeSet의 기본 이동속도에 적용
	NavigationComponent->SetControlledMonsterMoveSpeed(
		DesiredMoveSpeed
	);

	// 속도 적용이 끝났으므로 다음 Task로 진행
	return EBTNodeResult::Succeeded;
}