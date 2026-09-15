

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_BaruWaitForAmbushTrigger.generated.h"

class APawn;
class UBehaviorTreeComponent;

/**
 * 매복 위치에 도착한 몬스터를 대기시키는 태스크
 *
 * - 목표가 설정 거리 안으로 들어오면 기습 시작
 * - 최대 대기 시간이 지나면 매복 명령 취소
 * - 목표가 죽거나 사라져도 매복 명령 취소
 */
UCLASS()
class BARUGAME_API UBTTask_BaruWaitForAmbushTrigger : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_BaruWaitForAmbushTrigger();

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
	// 이 노드 인스턴스가 매복 상태로 기다린 시간
	float ElapsedWaitTime = 0.0f;

	// 대상이 살아 있는 플레이어인지 확인
	bool IsValidAmbushTarget(APawn* TargetPawn) const;

	// 매복 준비 및 기습 여부를 블랙보드에 기록
	void SetAmbushState(
		UBehaviorTreeComponent& OwnerComp,
		bool bIsReady,
		bool bShouldSpring
	) const;

	// 디렉터의 매복 명령을 취소
	void CancelAmbush(UBehaviorTreeComponent& OwnerComp) const;
};
