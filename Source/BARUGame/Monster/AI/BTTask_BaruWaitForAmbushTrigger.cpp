

#include "BTTask_BaruWaitForAmbushTrigger.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Interfaces/CombatInterface.h"
#include "Monster/AI/BaruMonsterAIController.h"
#include "Monster/Characters/BaruMonsterCharacter.h"
#include "Monster/Data/BaruMonsterDataAsset.h"

// 블랙보드 에셋과 이름이 반드시 같아야 하는 매복 관련 키
namespace BaruAmbushTaskKeys
{
    const FName AmbushTargetActor(TEXT("AmbushTargetActor"));
    const FName IsAmbushReady(TEXT("IsAmbushReady"));
    const FName ShouldSpringAmbush(TEXT("ShouldSpringAmbush"));
}

UBTTask_BaruWaitForAmbushTrigger::UBTTask_BaruWaitForAmbushTrigger()
{
    // Behavior Tree 편집기에 표시될 태스크 이름
    NodeName = TEXT("Wait For Ambush Trigger");

    // 플레이어와의 거리 및 최대 대기 시간을 지속적으로 확인하기 위해
    // Behavior Tree Task의 Tick을 활성화
    bNotifyTick = true;

    // Behavior Tree 노드는 기본적으로 여러 AI가 공유할 수 있음
    // ElapsedWaitTime이 몬스터끼리 섞이지 않도록
    // 각 Behavior Tree마다 별도의 태스크 인스턴스를 생성
    bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_BaruWaitForAmbushTrigger::ExecuteTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory
)
{
    // 이 Behavior Tree를 실행 중인 몬스터 AIController 확인
    ABaruMonsterAIController* MonsterAI =
        Cast<ABaruMonsterAIController>(OwnerComp.GetAIOwner());

    // AIController가 조종하고 있는 몬스터 확인
    ABaruMonsterCharacter* Monster =
        MonsterAI
            ? Cast<ABaruMonsterCharacter>(MonsterAI->GetPawn())
            : nullptr;

    // 매복 대상과 매복 상태를 읽고 기록할 블랙보드 확인
    UBlackboardComponent* Blackboard =
        OwnerComp.GetBlackboardComponent();

    // 필요한 객체가 없거나 현재 명령이 매복이 아니면
    // 이 태스크를 실행할 수 없음
    if (!IsValid(MonsterAI) ||
        !IsValid(Monster) ||
        !IsValid(Blackboard) ||
        MonsterAI->GetDirectorCommand() !=
            EBaruMonsterDirectorCommand::Ambush)
    {
        return EBTNodeResult::Failed;
    }

    // 디렉터가 지정한 매복 대상 플레이어를 블랙보드에서 가져옴
    APawn* TargetPawn = Cast<APawn>(
        Blackboard->GetValueAsObject(
            BaruAmbushTaskKeys::AmbushTargetActor
        )
    );

    // 대상이 사라졌거나 이미 죽었다면
    // 매복을 유지할 이유가 없으므로 명령 취소
    if (!IsValidAmbushTarget(TargetPawn))
    {
        CancelAmbush(OwnerComp);
        return EBTNodeResult::Failed;
    }

    // 이 몬스터 종류의 매복 설정을 DataAsset에서 가져옴
    const UBaruMonsterDataAsset* MonsterData =
        Monster->GetMonsterDataAsset();

    // DataAsset이 없거나 매복이 허용되지 않은 몬스터라면 취소
    if (!IsValid(MonsterData) || !MonsterData->bCanAmbush)
    {
        CancelAmbush(OwnerComp);
        return EBTNodeResult::Failed;
    }

    // 이번 매복의 대기 시간을 처음부터 계산
    ElapsedWaitTime = 0.0f;

    // EQS로 찾은 매복 지점에 도착했으므로
    // 남아 있는 이동 요청을 제거하고 그 자리에 정지
    MonsterAI->StopMovement();

    // 다른 시스템이나 디버깅 화면에서
    // 이 몬스터가 매복 준비를 끝냈다는 것을 확인할 수 있도록 기록
    SetAmbushState(
        OwnerComp,
        true,   // IsAmbushReady
        false   // ShouldSpringAmbush
    );

    // 이 몬스터 DataAsset에 설정된 기습 발동 거리
    const float TriggerDistance =
        FMath::Max(
            0.0f,
            MonsterData->AmbushTriggerDistance
        );

    // 제곱 거리를 사용해 불필요한 제곱근 계산을 피함
    const float DistanceSquared = FVector::DistSquared(
        Monster->GetActorLocation(),
        TargetPawn->GetActorLocation()
    );

    // 매복 위치에 도착한 시점에 플레이어가 이미 가까우면
    // 기다리지 않고 곧바로 다음 기습 단계로 이동
    if (DistanceSquared <= FMath::Square(TriggerDistance))
    {
        SetAmbushState(
            OwnerComp,
            false,  // 더 이상 대기 상태가 아님
            true    // 기습 시작 요청
        );

        return EBTNodeResult::Succeeded;
    }

    // 아직 플레이어가 멀리 있으므로
    // TickTask에서 거리와 시간을 계속 확인
    return EBTNodeResult::InProgress;
}

void UBTTask_BaruWaitForAmbushTrigger::TickTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory,
    float DeltaSeconds
)
{
    // 실행 도중에도 AIController와 몬스터가 유효한지 다시 확인
    // 몬스터가 사망하거나 제거될 수 있기 때문
    ABaruMonsterAIController* MonsterAI =
        Cast<ABaruMonsterAIController>(OwnerComp.GetAIOwner());

    ABaruMonsterCharacter* Monster =
        MonsterAI
            ? Cast<ABaruMonsterCharacter>(MonsterAI->GetPawn())
            : nullptr;

    UBlackboardComponent* Blackboard =
        OwnerComp.GetBlackboardComponent();

    // 태스크 실행에 필요한 객체가 사라졌다면 실패 처리
    if (!IsValid(MonsterAI) ||
        !IsValid(Monster) ||
        !IsValid(Blackboard))
    {
        FinishLatentTask(
            OwnerComp,
            EBTNodeResult::Failed
        );
        return;
    }

    // 기다리는 도중 디렉터가 다른 명령을 내렸거나
    // 매복 명령을 해제했다면 현재 태스크도 종료
    if (MonsterAI->GetDirectorCommand() !=
        EBaruMonsterDirectorCommand::Ambush)
    {
        SetAmbushState(
            OwnerComp,
            false,
            false
        );

        FinishLatentTask(
            OwnerComp,
            EBTNodeResult::Failed
        );
        return;
    }

    // 현재 매복 대상 플레이어를 다시 조회
    APawn* TargetPawn = Cast<APawn>(
        Blackboard->GetValueAsObject(
            BaruAmbushTaskKeys::AmbushTargetActor
        )
    );

    // 대상이 게임에서 나갔거나 죽었다면 매복 취소
    if (!IsValidAmbushTarget(TargetPawn))
    {
        /*
         * ClearDirectorCommand가 HasAmbushOrder를 false로 변경함
         * 그러면 Behavior Tree의 Blackboard 데코레이터가
         * 현재 매복 분기를 자동으로 중단시킴
         */
        CancelAmbush(OwnerComp);
        return;
    }

    // 매복 거리와 최대 대기 시간을 확인하기 위해
    // 현재 몬스터의 DataAsset을 다시 가져옴
    const UBaruMonsterDataAsset* MonsterData =
        Monster->GetMonsterDataAsset();

    // 실행 중 설정이 잘못되었거나 매복 허용이 꺼졌다면 취소
    if (!IsValid(MonsterData) || !MonsterData->bCanAmbush)
    {
        CancelAmbush(OwnerComp);
        return;
    }

    // 실제 프레임 경과 시간을 누적
    ElapsedWaitTime += DeltaSeconds;

    const float TriggerDistance =
        FMath::Max(
            0.0f,
            MonsterData->AmbushTriggerDistance
        );

    const float DistanceSquared = FVector::DistSquared(
        Monster->GetActorLocation(),
        TargetPawn->GetActorLocation()
    );

    // 플레이어가 기습 거리 안으로 들어왔는지 검사
    if (DistanceSquared <= FMath::Square(TriggerDistance))
    {
        // 대기 상태를 끝내고 다음 기습 실행 단계에 신호 전달
        SetAmbushState(
            OwnerComp,
            false,
            true
        );

        // Sequence의 다음 태스크가 실행되도록 성공 반환
        FinishLatentTask(
            OwnerComp,
            EBTNodeResult::Succeeded
        );
        return;
    }

    // 몬스터가 매복 위치에서 기다릴 수 있는 최대 시간
    // 잘못된 값으로 즉시 취소되지 않도록 최소 0.1초 보장
    const float MaximumWaitDuration =
        FMath::Max(
            0.1f,
            MonsterData->AmbushMaximumWaitDuration
        );

    // 플레이어가 오지 않은 채 최대 시간이 지났다면
    // 영원히 숨어 있지 않도록 매복 명령 취소
    if (ElapsedWaitTime >= MaximumWaitDuration)
    {
        CancelAmbush(OwnerComp);
    }
}

EBTNodeResult::Type UBTTask_BaruWaitForAmbushTrigger::AbortTask(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory
)
{
    /*
     * 플레이어 추적처럼 더 높은 우선순위의 분기가 실행되거나
     * Behavior Tree 자체가 중단되면 이 함수가 호출됨
     *
     * 매복 준비와 기습 신호가 블랙보드에 남지 않도록 정리
     */
    SetAmbushState(
        OwnerComp,
        false,
        false
    );

    return Super::AbortTask(
        OwnerComp,
        NodeMemory
    );
}

bool UBTTask_BaruWaitForAmbushTrigger::IsValidAmbushTarget(
    APawn* TargetPawn
) const
{
    // 대상이 제거됐거나 플레이어가 조종하는 Pawn이 아니면 제외
    if (!IsValid(TargetPawn) ||
        !TargetPawn->IsPlayerControlled())
    {
        return false;
    }

    // 전투 인터페이스를 가진 대상이라면 사망 여부까지 검사
    if (TargetPawn->Implements<UCombatInterface>() &&
        ICombatInterface::Execute_IsDead(TargetPawn))
    {
        return false;
    }

    return true;
}

void UBTTask_BaruWaitForAmbushTrigger::SetAmbushState(
    UBehaviorTreeComponent& OwnerComp,
    bool bIsReady,
    bool bShouldSpring
) const
{
    UBlackboardComponent* Blackboard =
        OwnerComp.GetBlackboardComponent();

    if (!IsValid(Blackboard))
    {
        return;
    }

    // 몬스터가 매복 위치에 도착해 기다리고 있는지 기록
    Blackboard->SetValueAsBool(
        BaruAmbushTaskKeys::IsAmbushReady,
        bIsReady
    );

    // 목표가 가까워져 기습을 시작해야 하는지 기록
    Blackboard->SetValueAsBool(
        BaruAmbushTaskKeys::ShouldSpringAmbush,
        bShouldSpring
    );
}

void UBTTask_BaruWaitForAmbushTrigger::CancelAmbush(
    UBehaviorTreeComponent& OwnerComp
) const
{
    // 남아 있는 매복 상태를 먼저 모두 초기화
    SetAmbushState(
        OwnerComp,
        false,
        false
    );

    // AIController의 디렉터 명령도 해제
    // 이 과정에서 HasAmbushOrder와 매복 대상도 정리됨
    if (ABaruMonsterAIController* MonsterAI =
        Cast<ABaruMonsterAIController>(OwnerComp.GetAIOwner()))
    {
        MonsterAI->ClearDirectorCommand();
    }
}
