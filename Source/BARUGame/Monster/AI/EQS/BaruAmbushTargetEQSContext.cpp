


#include "Monster/AI/EQS/BaruAmbushTargetEQSContext.h"

#include "Monster/AI/BaruMonsterAIController.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "GameFramework/Pawn.h"

namespace BaruAmbushContextBlackboardKeys
{
	const FName AmbushTargetActor(
		TEXT("AmbushTargetActor")
	);
}

void UBaruAmbushTargetEQSContext::ProvideContext(
	FEnvQueryInstance& QueryInstance,
	FEnvQueryContextData& ContextData
) const
{
	UObject* QueryOwner = QueryInstance.Owner.Get();

	if (!IsValid(QueryOwner))
	{
		return;
	}

	// Behavior Tree에서 EQS를 실행하면 QueryOwner가
	// AIController 또는 조종 중인 Pawn으로 들어올 수 있음
	ABaruMonsterAIController* MonsterController =
		Cast<ABaruMonsterAIController>(QueryOwner);

	if (!IsValid(MonsterController))
	{
		if (APawn* PawnOwner = Cast<APawn>(QueryOwner))
		{
			MonsterController =
				Cast<ABaruMonsterAIController>(
					PawnOwner->GetController()
				);
		}
	}

	if (!IsValid(MonsterController))
	{
		return;
	}

	UBlackboardComponent* MonsterBlackboard =
		MonsterController->GetBlackboardComponent();

	if (!IsValid(MonsterBlackboard))
	{
		return;
	}

	APawn* AmbushTarget =
		Cast<APawn>(
			MonsterBlackboard->GetValueAsObject(
				BaruAmbushContextBlackboardKeys::
					AmbushTargetActor
			)
		);

	if (!IsValid(AmbushTarget) ||
		!AmbushTarget->IsPlayerControlled())
	{
		return;
	}

	// Blackboard에서 가져온 목표 플레이어를
	// EQS의 Context Actor로 전달
	UEnvQueryItemType_Actor::SetContextHelper(
		ContextData,
		AmbushTarget
	);
}
