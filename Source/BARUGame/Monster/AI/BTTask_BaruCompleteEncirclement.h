

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_BaruCompleteEncirclement.generated.h"


/**
 * 포위 몬스터가 우회 경로와 차단 위치 이동을 완료했음을
 * AIController에 전달하는 Behavior Tree 태스크
 *
 * 이 태스크가 실행되면 포위 명령을 Hold 상태로 전환한다.
 */
UCLASS()
class BARUGAME_API UBTTask_BaruCompleteEncirclement
	: public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_BaruCompleteEncirclement();

protected:

	/**
	 * Behavior Tree에서 태스크가 실행될 때 호출
	 *
	 * 포위 명령을 수행 중인 AIController에
	 * 차단 위치 도착 완료를 전달한다.
	 */
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory
	) override;
};
