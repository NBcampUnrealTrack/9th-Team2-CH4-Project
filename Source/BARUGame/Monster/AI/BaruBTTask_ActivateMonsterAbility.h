

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "BaruBTTask_ActivateMonsterAbility.generated.h"

class UBehaviorTreeComponent;


UCLASS()
class BARUGAME_API UBaruBTTask_ActivateMonsterAbility : public UBTTaskNode
{
	GENERATED_BODY()
	
public:

	UBaruBTTask_ActivateMonsterAbility();

protected:

	// BT가 이 Task에 도달했을 때 지정된 Ability를 실행
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory
	) override;

private:

	// 실행할 Ability를 찾는 태그
	UPROPERTY(
		EditAnywhere,
		Category = "Monster|Ability"
	)
	FGameplayTag AbilityTag;
	
};
