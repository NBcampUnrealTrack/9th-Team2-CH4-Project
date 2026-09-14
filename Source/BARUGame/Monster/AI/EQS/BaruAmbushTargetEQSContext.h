

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "BaruAmbushTargetEQSContext.generated.h"


/**
 * 매복 EQS에 목표 플레이어를 제공하는 Context
 *
 * EQS를 실행한 몬스터의 Blackboard에서
 * AmbushTargetActor를 읽어 기준 Actor로 전달합니다.
 */
UCLASS()
class BARUGAME_API UBaruAmbushTargetEQSContext
	: public UEnvQueryContext
{
	GENERATED_BODY()

public:

	virtual void ProvideContext(
		FEnvQueryInstance& QueryInstance,
		FEnvQueryContextData& ContextData
	) const override;
};
