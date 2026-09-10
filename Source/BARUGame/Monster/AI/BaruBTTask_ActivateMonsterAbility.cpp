


#include "BaruBTTask_ActivateMonsterAbility.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

#include "Monster/Characters/BaruMonsterCharacter.h"
#include "Monster/Data/BaruMonsterDataAsset.h"
#include "AbilitySystem/BaruAbilitySystemComponent.h"

#include "BaruLog.h"


UBaruBTTask_ActivateMonsterAbility::UBaruBTTask_ActivateMonsterAbility()
{
    NodeName = TEXT("Activate Monster Ability");

    // 몬스터마다 별도의 Task 객체를 사용
    // 공격 상태와 대기시간이 다른 몬스터와 섞이지 않도록 함
    bCreateNodeInstance = true;

    // Task 실행 중 TickTask에서 공격 종료와 대기시간을 확인
    bNotifyTick = true;
}


EBTNodeResult::Type
UBaruBTTask_ActivateMonsterAbility::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory
)
{
    // 이전 실행에서 사용한 정보를 초기화
    CachedASC.Reset();
    ActiveAbilityHandle = FGameplayAbilitySpecHandle();
    AttackCooldown = 0.0f;
    RemainingCooldown = 0.0f;
    bWaitingForCooldown = false;

    // 몬스터 공격은 서버에서만 실행
    AAIController* OwnerController = OwnerComp.GetAIOwner();

    if (!IsValid(OwnerController) || !OwnerController->HasAuthority())
    {
        return EBTNodeResult::Failed;
    }

    // AIController가 조종하는 몬스터 확인
    ABaruMonsterCharacter* MonsterCharacter = 
        Cast<ABaruMonsterCharacter>(OwnerController->GetPawn());

    if (!IsValid(MonsterCharacter))
    {
        return EBTNodeResult::Failed;
    }

    // 실행할 Ability 태그 확인
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

    UBaruAbilitySystemComponent* MonsterASC =
        Cast<UBaruAbilitySystemComponent>(
            MonsterCharacter->GetAbilitySystemComponent()
        );

    const UBaruMonsterDataAsset* MonsterData =
        MonsterCharacter->GetMonsterDataAsset();

    if (!IsValid(MonsterASC) || !IsValid(MonsterData))
    {
        BARU_NET_LOG(
            OwnerController,
            LogBaruGAS,
            Warning,
            TEXT("Monster ASC or DataAsset is invalid.")
        );

        return EBTNodeResult::Failed;
    }

    // 공격이 끝난 뒤 기다릴 시간을 설정표에서 가져옴
    AttackCooldown =
        FMath::Max(0.0f, MonsterData->AttackCooldown);

    // 지정된 태그를 가진 Ability 하나를 찾음
    // 반복문 안에서는 실행하지 않고 식별 번호만 보관
    for (const FGameplayAbilitySpec& Spec :
        MonsterASC->GetActivatableAbilities())
    {
        if (!Spec.Ability)
        {
            continue;
        }

        const bool bHasAssetTag = Spec.Ability->GetAssetTags().HasTagExact(AbilityTag);

        const bool bHasDynamicTag = Spec.GetDynamicSpecSourceTags().HasTagExact(AbilityTag);

        if (bHasAssetTag || bHasDynamicTag)
        {
            // 다른 곳에서 이미 실행 중인 Ability는 맡지 않음
            if (Spec.IsActive())
            {
                return EBTNodeResult::Failed;
            }

            ActiveAbilityHandle = Spec.Handle;
            break;
        }
    }

    if (!ActiveAbilityHandle.IsValid())
    {
        BARU_NET_LOG(
            OwnerController,
            LogBaruGAS,
            Warning,
            TEXT("Monster Ability not found: %s"),
            *AbilityTag.ToString()
        );

        return EBTNodeResult::Failed;
    }

    CachedASC = MonsterASC;

    // 위에서 선택한 Ability 하나만 실행
    const bool bActivated =
        MonsterASC->TryActivateAbility(ActiveAbilityHandle);

    if (!bActivated)
    {
        BARU_NET_LOG(
            OwnerController,
            LogBaruGAS,
            Warning,
            TEXT("Failed to activate Monster Ability: %s"),
            *AbilityTag.ToString()
        );

        CachedASC.Reset();
        ActiveAbilityHandle = FGameplayAbilitySpecHandle();

        return EBTNodeResult::Failed;
    }

    // 바로 성공 처리하지 않고 TickTask에서 종료를 기다림
    return EBTNodeResult::InProgress;
}


void UBaruBTTask_ActivateMonsterAbility::TickTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory,
    float DeltaSeconds
)
{
    UBaruAbilitySystemComponent* MonsterASC = CachedASC.Get();

    // 대기 중 몬스터의 ASC가 사라졌다면 Task 실패 처리
    if (!IsValid(MonsterASC))
    {
        CachedASC.Reset();
        ActiveAbilityHandle = FGameplayAbilitySpecHandle();
        bWaitingForCooldown = false;

        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    if (!bWaitingForCooldown)
    {
        // 이번 Task가 실행한 Ability의 현재 상태 확인
        const FGameplayAbilitySpec* AbilitySpec =
            MonsterASC->FindAbilitySpecFromHandle(
                ActiveAbilityHandle
            );

        // Ability 자체가 제거됐다면 Task 중단
        if (!AbilitySpec)
        {
            CachedASC.Reset();
            ActiveAbilityHandle = FGameplayAbilitySpecHandle();

            FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
            return;
        }

        // 아직 공격 중이라면 계속 기다림
        if (AbilitySpec->IsActive())
        {
            return;
        }

        // 공격 종료를 확인한 순간부터 대기시간 계산 시작
        bWaitingForCooldown = true;
        RemainingCooldown = AttackCooldown;
    }
    else
    {
        // 공격 종료 후에만 남은 대기시간을 줄임
        RemainingCooldown =
            FMath::Max(0.0f, RemainingCooldown - DeltaSeconds);
    }

    if (RemainingCooldown > 0.0f)
    {
        return;
    }

    // 공격과 대기가 모두 끝났으므로 Task 완료
    CachedASC.Reset();
    ActiveAbilityHandle = FGameplayAbilitySpecHandle();
    bWaitingForCooldown = false;

    FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
}


EBTNodeResult::Type
UBaruBTTask_ActivateMonsterAbility::AbortTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory
)
{
    UBaruAbilitySystemComponent* MonsterASC = CachedASC.Get();

    const FGameplayAbilitySpecHandle AbilityToCancel =
        ActiveAbilityHandle;

    // 취소 과정에서 다른 콜백이 실행될 수 있으므로 먼저 정리
    CachedASC.Reset();
    ActiveAbilityHandle = FGameplayAbilitySpecHandle();
    bWaitingForCooldown = false;
    RemainingCooldown = 0.0f;

    // 이 Task가 실행한 Ability만 취소
    // 다른 Ability에는 영향을 주지 않음
    if (IsValid(MonsterASC) && AbilityToCancel.IsValid())
    {
        MonsterASC->CancelAbilityHandle(AbilityToCancel);
    }

    return EBTNodeResult::Aborted;
}