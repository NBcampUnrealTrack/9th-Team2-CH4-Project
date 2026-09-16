


#include "BTService_BaruMovementRecovery.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"

#include "BaruLog.h"

const FName UBTService_BaruMovementRecovery::IsMovementRecoveringKeyName(
    TEXT("IsMovementRecovering"));
const FName UBTService_BaruMovementRecovery::MovementRecoveryLocationKeyName(
    TEXT("MovementRecoveryLocation"));

UBTService_BaruMovementRecovery::UBTService_BaruMovementRecovery()
{
    // 생성자는 노드의 기본 설정을 준비한다. 실제 끼임 검사는 TickNode가 담당한다.
    NodeName = TEXT("Baru Movement Recovery");
    // 서비스의 타이머와 실패 기록을 몬스터별로 분리한다.
    // 공유 노드에 몬스터별 상태를 저장하면 서로의 위치/타이머를 덮어쓸 수 있다.
    bCreateNodeInstance = true;
    // 서비스의 주기적 TickNode 호출을 사용한다.
    bNotifyTick = true;
    // 기본 0.25초에 ±0.05초를 더해 갱신 시점을 분산한다.
    // 매 프레임 모든 몬스터가 경로를 검색하는 구조가 아니다.
    Interval = 0.25f;
    RandomDeviation = 0.05f;
}

void UBTService_BaruMovementRecovery::TickNode(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory,
    float DeltaSeconds)
{
    // 부모 클래스에 정의된 갱신 처리도 먼저 실행한다.
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    // 1. 실행 주체 확인.
    // Controller는 판단/명령 주체, Pawn은 실제 월드에서 움직이는 몸이다.
    AAIController* Controller = OwnerComp.GetAIOwner();
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    // ||는 하나라도 참이면 참이다. 앞쪽 조건이 참이면 뒤는 평가하지 않는다.
    // 따라서 Controller가 무효일 때 ->HasAuthority()를 호출하지 않는다.
    // 서버가 AI 이동을 결정하도록 권한 검사도 함께 한다.
    if (!IsValid(Controller) || !Controller->HasAuthority() || !IsValid(Blackboard))
    {
        bHasObservationAnchor = false;
        // void 함수의 조기 종료. 이번 검사만 끝내며 BT 전체를 종료하는 것은 아니다.
        return;
    }

    // 이 서비스는 CharacterMovement와 루트 캡슐을 사용하는 몬스터용이다.
    // Cast는 실제 Pawn이 ACharacter 계열일 때만 해당 타입의 포인터를 반환한다.
    ACharacter* Character = Cast<ACharacter>(Controller->GetPawn());
    if (!IsValid(Character) || !IsValid(Character->GetCharacterMovement()))
    {
        bHasObservationAnchor = false;
        return;
    }

    const FVector CurrentLocation = Character->GetActorLocation();
    UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
    UPathFollowingComponent* PathFollowing = Controller->GetPathFollowingComponent();
    // 비정상적인 음수 시간이 들어와 타이머가 거꾸로 진행하지 않도록 보정한다.
    const float SafeDelta = FMath::Max(0.0f, DeltaSeconds);

    // 2. 처음 관찰하거나 조종 중인 캐릭터가 바뀌면 이전 개체의 상태를 비운다.
    // 매 Tick 초기화하면 끼임 시간이 누적되지 않으므로 개체 변경 때만 실행한다.
    if (ObservedCharacter.Get() != Character)
    {
        ObservedCharacter = Character;
        bWaitingForRetry = false;
        bResetCandidatesAfterWait = false;
        RecoveryElapsedTime = 0.0f;
        RetryDelayRemaining = 0.0f;
        ConsecutiveFailures = 0;
        FailedRecoveryLocations.Reset();
        ResetMovementObservation(CurrentLocation);
        Blackboard->SetValueAsBool(IsMovementRecoveringKeyName, false);
        Blackboard->ClearValue(MovementRecoveryLocationKeyName);
    }

    // 낙하·점프 중에는 지상 제자리걸음으로 판단하지 않는다.
    // 사망 등으로 이동 자체가 꺼졌다면 복구 명령도 중단한다.
    if (!Movement->IsMovingOnGround())
    {
        // 공중에서 보낸 시간을 지상 막힘으로 이어서 세지 않도록 기준점만 갱신한다.
        ResetMovementObservation(CurrentLocation);
        if (Movement->MovementMode == MOVE_None &&
            Blackboard->GetValueAsBool(IsMovementRecoveringKeyName))
        {
            Controller->StopMovement();
            Blackboard->SetValueAsBool(IsMovementRecoveringKeyName, false);
            Blackboard->ClearValue(MovementRecoveryLocationKeyName);
        }
        return;
    }

    // 3. 이미 복구 중이면 복구 전용 분기만 실행한다.
    // 여기서 매 경로가 return하므로 아래의 '새 끼임 감지'와 동시에 실행되지 않는다.
    if (Blackboard->GetValueAsBool(IsMovementRecoveringKeyName))
    {
        if (bWaitingForRetry)
        {
            // 남은 시간을 깎는 카운트다운. 아직 양수면 BT의 Wait 상태를 유지한다.
            RetryDelayRemaining -= SafeDelta;
            if (RetryDelayRemaining > 0.0f)
            {
                return;
            }

            // 장애물이 움직일 시간을 준 뒤 이전에 막힌 방향도 다시 허용한다.
            if (bResetCandidatesAfterWait)
            {
                FailedRecoveryLocations.Reset();
                ConsecutiveFailures = 0;
                bResetCandidatesAfterWait = false;
            }
            TryStartRecoveryMove(*Controller, *Blackboard, *Character);
            // *포인터는 그 포인터가 가리키는 객체를 뜻한다.
            // 이 함수는 객체 참조(& 인자)를 받으므로 역참조해서 전달한다.
            return;
        }

        // 재시도 대기가 아니라 목적지 이동 단계일 때만 이동 시간을 누적한다.
        RecoveryElapsedTime += SafeDelta;
        const float CompletionRadius = FMath::Clamp(RecoveryCompletionRadius, 1.0f, 100.0f);
        const FVector FeetLocation = Character->GetNavAgentLocation();

        // 실제 도착 + 시작점에서의 이동을 함께 확인한다.
        // Idle/Waiting/Paused, 시간 초과는 도착 성공으로 처리하지 않는다.
        // &&로 묶인 세 조건을 모두 만족해야 한다.
        // (1) 수평 거리가 완료 반경 이내: 목적지 가까이 왔는가?
        // (2) 발밑 높이 차이가 허용 범위 이내: 다른 높이에서 X/Y만 겹친 것은 아닌가?
        // (3) 시작점에서 충분히 이동: 제자리에서 완료됐다고 오인하지 않는가?
        // DistSquared2D는 X/Y 거리의 제곱이다. 반경도 제곱해 비교하므로 제곱근이 필요 없다.
        const bool bReached =
            FVector::DistSquared2D(CurrentLocation, ActiveRecoveryLocation) <=
                FMath::Square(CompletionRadius) &&
            FMath::Abs(FeetLocation.Z - ActiveRecoveryLocation.Z) <=
                FMath::Max(20.0f, Movement->MaxStepHeight) &&
            FVector::DistSquared2D(CurrentLocation, RecoveryStartLocation) >=
                FMath::Square(FMath::Max(20.0f, MinimumMovementDistance));

        if (bReached)
        {
            FinishMovementRecovery(*Controller, *Blackboard, CurrentLocation);
            return;
        }

        // 복구 Selector의 Wait가 끝나고 Move To가 시작될 시간을 확보한다.
        // BT Wait는 0.5초로 설정한다.
        constexpr float MoveStartupGrace = 1.0f;
        // constexpr는 여기서 사용할 고정 상수를 뜻한다.
        // Idle은 '도착했다'가 아니라 '진행 중인 이동 요청이 없다'는 상태이므로,
        // 위의 도착 검사에 실패했다면 시작 유예 시간 후 실패로 취급한다.
        const bool bIdleBeforeArrival = RecoveryElapsedTime >= MoveStartupGrace &&
            (!IsValid(PathFollowing) || PathFollowing->GetStatus() == EPathFollowingStatus::Idle);

        // Moving이어도 실제 위치가 바뀌지 않으면 막힐 수 있다.
        // &&의 단락 평가 덕분에 이동 중일 때만 관찰 시간을 누적한다.
        const bool bStalled = IsValid(PathFollowing) &&
            PathFollowing->GetStatus() == EPathFollowingStatus::Moving &&
            HasStayedNearAnchor(CurrentLocation, SafeDelta);

        // 실패 이유를 하나 골라 다음 후보 재검색을 예약한다.
        // else if이므로 한 Tick에 실패 횟수가 여러 번 증가하지 않는다.
        if (bIdleBeforeArrival)
        {
            ScheduleRecoveryRetry(*Controller, *Blackboard, TEXT("IdleBeforeArrival"));
        }
        else if (bStalled)
        {
            ScheduleRecoveryRetry(*Controller, *Blackboard, TEXT("NoDisplacement"));
        }
        else if (RecoveryElapsedTime >= FMath::Max(1.5f, RecoveryTimeout))
        {
            ScheduleRecoveryRetry(*Controller, *Blackboard, TEXT("Timeout"));
        }
        return;
    }

    // 4. 복구 중이 아니면 일반 이동에서 새 끼임을 감지한다.
    // 공격·대기처럼 이동 요청이 없는 시간은 정지 시간에 포함하지 않는다.
    if (!IsValid(PathFollowing) || PathFollowing->GetStatus() != EPathFollowingStatus::Moving)
    {
        ResetMovementObservation(CurrentLocation);
        return;
    }

    if (!HasStayedNearAnchor(CurrentLocation, SafeDelta))
    {
        // 충분히 움직였거나, 아직 감지 시간이 쌓이지 않았다면 복구할 필요가 없다.
        return;
    }

    // StopMovement 이전에 원래 가려던 방향을 저장해야 후퇴 방향을 알 수 있다.
    // 목적지 - 현재 위치는 목적지를 향하는 벡터다.
    // GetSafeNormal2D는 높이를 제외한 단위 방향을 구하며 길이가 0인 경우도 처리한다.
    BlockedMoveDirection =
        (PathFollowing->GetCurrentTargetLocation() - CurrentLocation).GetSafeNormal2D();
    if (BlockedMoveDirection.IsNearlyZero())
    {
        // 경로 목표와 같은 위치라 방향을 얻지 못하면 몸이 바라보는 방향을 사용한다.
        BlockedMoveDirection = Character->GetActorForwardVector().GetSafeNormal2D();
    }
    if (BlockedMoveDirection.IsNearlyZero())
    {
        // 마지막 기본값: 월드 +X 방향. 영벡터로 옆 방향을 계산하지 않도록 한다.
        BlockedMoveDirection = FVector::ForwardVector;
    }

    // 5. 새 복구 작업 시작. 앞선 복구의 실패 기록을 지우고 이번 작업을 준비한다.
    ConsecutiveFailures = 0;
    FailedRecoveryLocations.Reset();
    bResetCandidatesAfterWait = false;
    bWaitingForRetry = true;
    Controller->StopMovement();
    Blackboard->ClearValue(MovementRecoveryLocationKeyName);
    // Bool이 켜지면 BT의 최우선 복구 분기가 선택된다.
    // 목적지 키가 아직 없으면 안내한 BT의 Wait가 실행된다.
    Blackboard->SetValueAsBool(IsMovementRecoveringKeyName, true);
    TryStartRecoveryMove(*Controller, *Blackboard, *Character);
}

bool UBTService_BaruMovementRecovery::HasStayedNearAnchor(
    const FVector& Location, float DeltaSeconds)
{
    // 저장된 에디터 값이 너무 작아도 최소 20cm는 기준점에서 벗어나야 한다.
    const float RequiredDisplacement = FMath::Max(20.0f, MinimumMovementDistance);
    if (!bHasObservationAnchor ||
        FVector::DistSquared2D(Location, ObservationAnchor) >= FMath::Square(RequiredDisplacement))
    {
        // 최초 관찰 또는 정상 이동: 기준점을 새 위치로 옮기고 타이머를 다시 시작한다.
        ResetMovementObservation(Location);
        return false;
    }

    // 매 검사마다 Anchor를 덮어쓰지 않는다. 작은 왕복 이동을 걸음으로 세지 않는다.
    // 예: 기준점 X=0, 문턱 거리 40일 때 X=5, -5, 5를 반복해도 계속 근처에 머문 것이다.
    StationaryElapsedTime += DeltaSeconds;
    return StationaryElapsedTime >= FMath::Max(0.1f, StuckDetectionDuration);
}

void UBTService_BaruMovementRecovery::ResetMovementObservation(const FVector& Location)
{
    // 관찰 기준점과 '그 근처에서 보낸 시간'은 항상 함께 초기화한다.
    ObservationAnchor = Location;
    StationaryElapsedTime = 0.0f;
    bHasObservationAnchor = true;
}

void UBTService_BaruMovementRecovery::TryStartRecoveryMove(
    AAIController& Controller,
    UBlackboardComponent& Blackboard,
    const ACharacter& Character)
{
    // FindRecoveryLocation이 true를 반환한 경우에만 이 변수에 유효한 결과가 들어 있다.
    FVector RecoveryLocation;
    if (!FindRecoveryLocation(Character, RecoveryLocation))
    {
        ScheduleRecoveryRetry(Controller, Blackboard, TEXT("NoClearCandidate"));
        return;
    }

    // 새 목적지를 지정했으므로 이번 이동의 출발점과 타이머도 새로 기록한다.
    ActiveRecoveryLocation = RecoveryLocation;
    RecoveryStartLocation = Character.GetActorLocation();
    RecoveryElapsedTime = 0.0f;
    bWaitingForRetry = false;
    ResetMovementObservation(RecoveryStartLocation);

    // 이동 자체는 BT Move To가 실행한다. 서비스에서 MoveTo를 중복 요청하지 않는다.
    // Blackboard 값 변경을 관찰하는 데코레이터/Move To가 새 위치를 받아 실행한다.
    Blackboard.SetValueAsVector(MovementRecoveryLocationKeyName, RecoveryLocation);
    // started는 목적지 배정 로그다. 실제 도착 성공은 reached 로그로 따로 확인한다.
    // &Controller는 참조로 받은 객체의 주소를 얻는다.
    // *RecoveryLocation.ToString()은 FString에서 로그용 문자 포인터를 얻는 표기다.
    BARU_NET_LOG((&Controller), LogBaruAI, Log,
        TEXT("Movement recovery started. Pawn: %s / Goal: %s / Failures: %d"),
        *GetNameSafe(&Character), *RecoveryLocation.ToString(), ConsecutiveFailures);
}

void UBTService_BaruMovementRecovery::ScheduleRecoveryRetry(
    AAIController& Controller,
    UBlackboardComponent& Blackboard,
    const TCHAR* Reason)
{
    // 이동 단계에서 실패한 경우에만 실제로 시도했던 목적지를 기록한다.
    // 후보를 못 찾은 경우에는 기록할 목적지가 없으므로 추가하지 않는다.
    if (!bWaitingForRetry)
    {
        FailedRecoveryLocations.Add(ActiveRecoveryLocation);
    }
    ++ConsecutiveFailures;
    // ++는 실패 횟수를 1 증가시킨다. 이후에는 이동 단계가 아닌 대기 단계로 처리한다.
    bWaitingForRetry = true;
    bResetCandidatesAfterWait =
        ConsecutiveFailures >= FMath::Clamp(MaximumConsecutiveFailures, 1, 8);

    // 같은 프레임에 부딪힌 개체들이 동시에 다시 출발하지 않도록 조금 분산한다.
    // % 4는 0~3의 나머지, 여기에 0.1을 곱하면 0~0.3초다.
    // 조건 ? A : B는 조건이 참이면 A, 거짓이면 B를 선택하는 삼항 연산자다.
    const APawn* Pawn = Controller.GetPawn();
    const float Stagger = IsValid(Pawn) ? static_cast<float>(Pawn->GetUniqueID() % 4) * 0.1f : 0.0f;
    RetryDelayRemaining = (bResetCandidatesAfterWait
        ? FMath::Max(1.0f, RecoveryBlockedWait)
        : FMath::Max(0.5f, RecoveryRetryDelay)) + Stagger;
    RecoveryElapsedTime = 0.0f;

    // 목적지 키가 없는 동안 Move To의 데코레이터가 이동을 막고 Wait가 실행된다.
    // 복구 Bool은 유지해야 하위 Chase 분기로 떨어지지 않는다.
    // ClearValue는 (0,0,0)을 목적지로 지정하는 것이 아니라 키를 미설정 상태로 만든다.
    Controller.StopMovement();
    Blackboard.ClearValue(MovementRecoveryLocationKeyName);

    BARU_NET_LOG((&Controller), LogBaruAI, Log,
        TEXT("Movement recovery retry. Pawn: %s / Reason: %s / Wait: %.2f / Failures: %d"),
        *GetNameSafe(Pawn), Reason, RetryDelayRemaining, ConsecutiveFailures);
}

void UBTService_BaruMovementRecovery::FinishMovementRecovery(
    AAIController& Controller,
    UBlackboardComponent& Blackboard,
    const FVector& CurrentLocation)
{
    // 이 함수는 TickNode의 실제 도착 조건을 통과했을 때 호출된다.
    // 시간 초과/이동 실패 경로에서는 호출하지 않는다.
    // Bool을 끄기 전에 기존 복구 이동을 중단한다.
    // Bool 변경으로 시작될 다음 추적 요청을 뒤늦게 StopMovement로 취소하지 않는다.
    Controller.StopMovement();
    Blackboard.SetValueAsBool(IsMovementRecoveringKeyName, false);
    Blackboard.ClearValue(MovementRecoveryLocationKeyName);
    bWaitingForRetry = false;
    bResetCandidatesAfterWait = false;
    RecoveryElapsedTime = 0.0f;
    ConsecutiveFailures = 0;
    FailedRecoveryLocations.Reset();
    ResetMovementObservation(CurrentLocation);

    BARU_NET_LOG((&Controller), LogBaruAI, Log,
        TEXT("Movement recovery reached. Pawn: %s / Location: %s"),
        *GetNameSafe(Controller.GetPawn()), *CurrentLocation.ToString());
}

bool UBTService_BaruMovementRecovery::FindRecoveryLocation(
    const ACharacter& Character, FVector& OutLocation) const
{
    // 이번 함수는 bool 반환형이다. 필수 객체가 없거나 후보가 없으면 return false;다.
    // TickNode 같은 void 함수의 return;과 구분한다.
    UWorld* World = Character.GetWorld();
    AAIController* Controller = Cast<AAIController>(Character.GetController());
    const UCapsuleComponent* Capsule = Character.GetCapsuleComponent();
    if (!IsValid(World) || !IsValid(Controller) || !IsValid(Capsule))
    {
        return false;
    }

    // 현재 월드의 내비게이션 시스템을 얻는다. NavMesh 투영과 경로 검색에 사용한다.
    UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    if (!IsValid(NavSystem))
    {
        return false;
    }

    // 캐릭터의 내비게이션 기준 위치를 사용한다. 지상 Character는 발밑 위치가 기준이다.
    const FVector Start = Character.GetNavAgentLocation();
    const float Completion = FMath::Clamp(RecoveryCompletionRadius, 1.0f, 100.0f);
    // FMath::Max는 두 값 중 큰 값을 선택한다.
    // 설정값이 잘못되어도 '완료 반경 + 실제 이동 문턱 + 캡슐 반경'보다 먼 후보만 허용한다.
    const float MinimumDistance = FMath::Max(MinimumRecoveryDistance,
        Completion + FMath::Max(20.0f, MinimumMovementDistance) + Capsule->GetScaledCapsuleRadius());
    // 실패할수록 기본 탐색 거리를 60cm씩 늘리고 100~600cm로 제한한다.
    const float Radius = FMath::Clamp(RecoverySearchRadius + ConsecutiveFailures * 60.0f, 100.0f, 600.0f);
    const float SideSign = ((Character.GetUniqueID() + ConsecutiveFailures) % 2 == 0) ? 1.0f : -1.0f;
    // 외적(CrossProduct)으로 위 방향과 진행 방향에 수직인 옆 방향을 구한다.
    // SideSign이 -1이면 반대쪽이 된다. 실패 횟수에 따라 먼저 검사하는 쪽이 교대한다.
    const FVector Side = FVector::CrossProduct(FVector::UpVector, BlockedMoveDirection) * SideSign;

    // 옆과 뒤를 먼저 검사한다. 개체와 실패 횟수에 따라 좌우 순서를 바꾼다.
    const FVector Directions[] = {
        Side, -Side,
        (Side - BlockedMoveDirection).GetSafeNormal2D(),
        (-Side - BlockedMoveDirection).GetSafeNormal2D(),
        -BlockedMoveDirection,
        (Side + BlockedMoveDirection).GetSafeNormal2D(),
        (-Side + BlockedMoveDirection).GetSafeNormal2D(),
        BlockedMoveDirection
    };
    // 같은 8방향을 기본 거리, 짧은 거리, 긴 거리 순으로 검사할 수 있다.
    // 기본 Attempts=16이면 처음 두 거리 묶음까지만 검사한다.
    const float DistanceScales[] = { 1.0f, 0.65f, 1.5f };
    const int32 Attempts = FMath::Clamp(RecoveryPointSearchAttempts, 1, 24);

    for (int32 Index = 0; Index < Attempts; ++Index)
    {
        // 정수 나눗셈 Index/8: 0~7은 0, 8~15는 1, 16~23은 2번 거리 배율.
        // 나머지 Index%8: 각 거리 묶음 안에서 사용할 0~7번 방향.
        // 위치 = 시작점 + 단위 방향 * 거리. 월드 좌표로 후보를 만드는 식이다.
        const float Distance = FMath::Min(600.0f, Radius * DistanceScales[Index / 8]);
        const FVector RawLocation = Start + Directions[Index % 8] * Distance;
        FNavLocation Projected;

        // 캐릭터의 NavAgent 설정으로 투영한다. 벽 건너편까지 넓게 투영하지 않는다.
        // ProjectPointToNavigation은 후보 주변의 NavMesh 위치를 찾아 Projected에 쓴다.
        // 성공했다고 현재 위치에서 그곳까지 갈 수 있다는 뜻은 아니므로 경로도 따로 검사한다.
        if (!NavSystem->ProjectPointToNavigation(RawLocation, Projected,
            FVector(50.0f, 50.0f, 100.0f), &Character.GetNavAgentPropertiesRef()))
        {
            // continue는 함수 종료가 아니라 이 후보만 건너뛰고 다음 반복으로 넘어간다.
            continue;
        }
        const FVector Candidate = Projected.Location;
        if (FVector::DistSquared2D(Start, Candidate) < FMath::Square(MinimumDistance))
        {
            continue;
        }

        // 실패했던 정확한 한 점뿐 아니라 100cm 근처 후보도 피한다.
        // 거의 같은 지점을 조금씩 바꾸며 같은 장애물을 다시 미는 현상을 줄인다.
        bool bRecentlyFailed = false;
        for (const FVector& FailedLocation : FailedRecoveryLocations)
        {
            if (FVector::DistSquared2D(Candidate, FailedLocation) < FMath::Square(100.0f))
            {
                bRecentlyFailed = true;
                // break는 이 안쪽 실패 기록 반복문만 종료한다.
                break;
            }
        }
        if (bRecentlyFailed)
        {
            continue;
        }

        // 동기 경로 검색: 이 호출이 끝나면 결과를 바로 확인할 수 있다.
        // 매 Tick 수행하지 않고 복구 후보가 필요할 때, 제한된 횟수만 검사한다.
        UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
            World, Start, Candidate, Controller, nullptr);
        // IsValid(Path): UObject 포인터 자체가 유효한가?
        // Path->IsValid(): 경로 데이터가 유효한가?
        // IsPartial(): 목적지에 도달하지 못하고 중간까지만 이어지는 경로인가?
        // 최소 두 점이 있어야 시작에서 다음 지점으로 이어지는 구간을 검사할 수 있다.
        if (!IsValid(Path) || !Path->IsValid() || Path->IsPartial() || Path->PathPoints.Num() < 2)
        {
            continue;
        }
        const double PathLength = Path->GetPathLength();
        // 비정상 수치, 길이 0, 지나치게 먼 우회, 실제 캡슐 충돌 중 하나라도 있으면 제외한다.
        if (!FMath::IsFinite(PathLength) || PathLength <= 0.0 || PathLength > Radius * 3.0f ||
            !IsRecoveryPathClear(Character, *Path))
        {
            continue;
        }

        // 가장 짧은 경로로 다시 덮어쓰지 않는다. 앞서 검사한 안전한 옆길을 사용한다.
        // 출력 참조에 좌표를 써 준 다음 true로 '결과가 준비됐다'고 알린다.
        OutLocation = Candidate;
        return true;
    }
    // 모든 후보를 검사했지만 선택하지 못했다. 호출자가 대기 후 재검색을 예약한다.
    return false;
}

bool UBTService_BaruMovementRecovery::IsRecoveryPathClear(
    const ACharacter& Character, const UNavigationPath& Path) const
{
    // NavMesh는 경로가 존재한다고 판단해도, 그 위에 동적 장애물이 서 있을 수 있다.
    // 이 함수는 캐릭터 몸 크기의 캡슐로 현재 실제 충돌 공간을 추가 검사한다.
    UWorld* World = Character.GetWorld();
    const UCapsuleComponent* Capsule = Character.GetCapsuleComponent();
    if (!IsValid(World) || !IsValid(Capsule) || Path.PathPoints.Num() < 2)
    {
        return false;
    }

    // 바닥 접촉 오차를 위한 1cm 여유만 사용한다. 벽을 통과하도록 충돌을 끄지 않는다.
    // Scaled 크기를 사용해야 에디터에서 캐릭터 크기를 조정한 경우도 반영된다.
    // HalfHeight는 전체 높이의 절반이며, 캡슐 반경보다 작아지지 않도록 보정한다.
    constexpr float Skin = 1.0f;
    const float Radius = FMath::Max(1.0f, Capsule->GetScaledCapsuleRadius() - Skin);
    const float HalfHeight = FMath::Max(Radius, Capsule->GetScaledCapsuleHalfHeight() - Skin);
    const FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
    // Rotation은 검사 캡슐의 회전, Channel은 실제 캡슐의 충돌 Object Type이다.
    const FQuat Rotation = Capsule->GetComponentQuat();
    const ECollisionChannel Channel = Capsule->GetCollisionObjectType();
    // QueryParams: 검사 이름, 단순 충돌 사용(false), 자기 자신은 검사에서 제외.
    // ResponseParams: 실제 캡슐의 채널별 Block/Overlap/Ignore 응답 설정을 사용한다.
    // 따라서 다른 몬스터가 모두 무조건 장애물인 것은 아니고 실제 충돌 설정을 따른다.
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BaruMovementRecovery), false, &Character);
    const FCollisionResponseParams ResponseParams(Capsule->GetCollisionResponseToChannels());

    // PathPoints는 발밑 NavMesh 위치이므로 캡슐 중심 높이로 변환한다.
    // 출발점은 실제 몸의 중심이고, 다음 경로점부터 발밑 좌표에 중심 높이를 더한다.
    const FVector CenterOffset(0.0f, 0.0f, Capsule->GetScaledCapsuleHalfHeight() + Skin);
    FVector SegmentStart = Capsule->GetComponentLocation() + FVector(0.0f, 0.0f, Skin);

    for (int32 Index = 1; Index < Path.PathPoints.Num(); ++Index)
    {
        const FVector SegmentEnd = Path.PathPoints[Index] + CenterOffset;
        // Overlap: 이 지점에 캡슐을 놓았을 때 막는 물체와 겹치는가?
        if (World->OverlapBlockingTestByChannel(SegmentEnd, Rotation, Channel,
            Shape, QueryParams, ResponseParams))
        {
            return false;
        }

        FHitResult Hit;
        // Sweep: 캡슐을 현재 구간의 시작에서 끝까지 훑었을 때 중간에 막히는가?
        // 실제 캐릭터를 이동시키는 호출이 아니라 충돌을 질의하는 호출이다.
        if (World->SweepSingleByChannel(Hit, SegmentStart, SegmentEnd, Rotation,
            Channel, Shape, QueryParams, ResponseParams))
        {
            // 벽·회전 장애물·다른 몬스터가 실제 캡슐을 막으면 후보를 버린다.
            // 시작부터 깊게 겹친 경우도 안전하다고 간주하지 않는다.
            return false;
        }
        // 이번 구간의 끝을 다음 구간의 시작으로 삼아 전체 경로를 차례로 검사한다.
        SegmentStart = SegmentEnd;
    }
    // 검사한 모든 구간이 현재 충돌 질의를 통과했다.
    // 검사 이후 움직여 들어온 장애물이나 실제 이동 중 실패는 TickNode가 다시 감지한다.
    return true;
}