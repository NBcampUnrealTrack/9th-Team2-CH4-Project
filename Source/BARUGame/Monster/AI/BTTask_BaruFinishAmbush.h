

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_BaruFinishAmbush.generated.h"

/**
 * 매복 돌진이 끝난 뒤 디렉터의 매복 명령을 정리하는 태스크
 *
 * 매복 관련 블랙보드 값을 초기화하고
 * 몬스터를 일반적인 개별 AI 판단으로 복귀시킵니다.
 */
UCLASS()
class BARUGAME_API UBTTask_BaruFinishAmbush : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_BaruFinishAmbush();

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory
	) override;
};
