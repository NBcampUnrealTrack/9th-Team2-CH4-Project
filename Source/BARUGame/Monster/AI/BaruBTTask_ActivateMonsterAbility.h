

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpecHandle.h"
#include "BaruBTTask_ActivateMonsterAbility.generated.h"

class UBehaviorTreeComponent;
class UBaruAbilitySystemComponent;

UCLASS()
class BARUGAME_API UBaruBTTask_ActivateMonsterAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UBaruBTTask_ActivateMonsterAbility();

protected:

	// 공격 Ability를 실행하고 대기 시작
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory
	) override;

	// Task가 실행 중일 때 공격 종료와 대기시간을 확인
	virtual void TickTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory,
		float DeltaSeconds
	) override;

	// 추적 대상 상실 등으로 BT가 이 Task를 중단할 때 처리
	virtual EBTNodeResult::Type AbortTask(
		UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory
	) override;

private:

	// 실행할 Ability를 찾는 태그
	UPROPERTY(EditAnywhere, Category = "Monster|Ability")
	FGameplayTag AbilityTag;

	// 현재 공격을 실행한 몬스터의 ASC
	// 몬스터가 제거되면 유효하지 않은 참조로 판단할 수 있도록 보관
	TWeakObjectPtr<UBaruAbilitySystemComponent> CachedASC;

	// 이번 Task가 실행한 Ability를 식별하는 번호
	FGameplayAbilitySpecHandle ActiveAbilityHandle;

	// DataAsset에서 읽은 공격 후 대기시간
	float AttackCooldown = 0.0f;

	// 공격이 끝난 뒤 남은 대기시간
	float RemainingCooldown = 0.0f;

	// 공격 종료 후 대기시간을 계산 중인지 여부
	bool bWaitingForCooldown = false;
};