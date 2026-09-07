


#include "BaruBTTask_ActivateMonsterAbility.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

#include "Monster/Characters/BaruMonsterCharacter.h"
#include "AbilitySystem/BaruAbilitySystemComponent.h"

#include "BaruLog.h"


UBaruBTTask_ActivateMonsterAbility::UBaruBTTask_ActivateMonsterAbility()
{
    NodeName = TEXT("Activate Monster Ability");
}

EBTNodeResult::Type
UBaruBTTask_ActivateMonsterAbility::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory
)
{
    // 이 BT를 실행 중인 AIController를 가져옴
    AAIController* OwnerController = OwnerComp.GetAIOwner();

    // 몬스터의 Ability 실행은 서버에서만 처리
    if (!IsValid(OwnerController) || !OwnerController->HasAuthority())
    {
        return EBTNodeResult::Failed;
    }

    // AIController가 조종 중인 몬스터를 가져옴
    ABaruMonsterCharacter* MonsterCharacter =
        Cast<ABaruMonsterCharacter>(
        OwnerController->GetPawn()
    );

    if (!IsValid(MonsterCharacter))
    {
        return EBTNodeResult::Failed;
    }

    // 이 Task에 실행할 Ability 태그가 지정됐는지 확인
    if (!AbilityTag.IsValid())
    {
        BARU_NET_LOG(
            OwnerController,
            LogBaruAI,
            Warning,
            TEXT("Monster Ability Tag is invalid.")
        );

        return EBTNodeResult::Failed;
    }

    // 몬스터가 소유한 BARU ASC를 가져옴
    UBaruAbilitySystemComponent* MonsterASC =
        Cast<UBaruAbilitySystemComponent>(
            MonsterCharacter->GetAbilitySystemComponent()
        );

    if (!IsValid(MonsterASC))
    {
        BARU_NET_LOG(
            OwnerController,
            LogBaruGAS,
            Error,
            TEXT("Monster ASC is invalid.")
        );

        return EBTNodeResult::Failed;
    }

    // 같은 태그가 지정된 Ability를 찾아 실행
    const bool bActivated =
        MonsterASC->TryActivateAbilityByTag(
            AbilityTag
        );

    if (!bActivated)
    {
        BARU_NET_LOG(
            OwnerController,
            LogBaruGAS,
            Warning,
            TEXT(
                "Failed to activate Monster Ability: %s"
            ),
            *AbilityTag.ToString()
        );

        return EBTNodeResult::Failed;
    }

    return EBTNodeResult::Succeeded;
}