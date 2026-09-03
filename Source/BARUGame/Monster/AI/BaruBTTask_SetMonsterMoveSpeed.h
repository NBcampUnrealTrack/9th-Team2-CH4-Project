

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BaruBTTask_SetMonsterMoveSpeed.generated.h"

class UBehaviorTreeComponent;

// BT에서 선택할 몬스터 이동속도 종류
UENUM(BlueprintType)
enum class EBaruMonsterMoveSpeedMode : uint8
{
	Patrol UMETA(DisplayName = "Patrol"),
	Chase UMETA(DisplayName = "Chase")
};

UCLASS()
class BARUGAME_API UBaruBTTask_SetMonsterMoveSpeed : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBaruBTTask_SetMonsterMoveSpeed();

protected:

	// BT가 이 Task에 도달했을 때 한 번 실행
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory
	) override;

private:

	// 이 노드에서 적용할 이동속도 종류
	UPROPERTY(EditAnywhere, Category = "Monster|Movement")
	EBaruMonsterMoveSpeedMode MoveSpeedMode =
		EBaruMonsterMoveSpeedMode::Chase;
	
};
