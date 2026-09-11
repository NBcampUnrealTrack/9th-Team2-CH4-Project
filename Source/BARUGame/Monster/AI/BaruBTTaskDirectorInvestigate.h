

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "BaruBTTaskDirectorInvestigate.generated.h"

class UAITask_MoveTo;
struct FAIMoveRequest;

UCLASS()
class BARUGAME_API UBaruBTTaskDirectorInvestigate : public UBTTask_MoveTo
{
	GENERATED_BODY()
	
public:
	UBaruBTTaskDirectorInvestigate();

protected:
	// 현재 조사 명령을 확인한 뒤 부모의 이동 처리 실행
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory
	) override;

	// 실제 이동 요청에 개체별 층 제한 필터 적용
	virtual UAITask_MoveTo* PrepareMoveTask(
		UBehaviorTreeComponent& OwnerComp,
		UAITask_MoveTo* ExistingTask,
		FAIMoveRequest& MoveRequest
	) override;

	// 도착·실패·중단 시 명령 정리 여부 판단
	virtual void OnTaskFinished(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		EBTNodeResult::Type TaskResult
	) override;

private:
	// 이번 실행에서 접수한 명령 번호
	uint32 ExecutingCommandRevision = 0;

	// 유효한 명령으로 실행을 시작했는지
	bool bHasExecutingCommand = false;
	
};
