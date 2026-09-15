


#include "BTService_BaruMovementRecovery.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"

#include "BaruLog.h"


const FName
UBTService_BaruMovementRecovery::IsMovementRecoveringKeyName(
    TEXT("IsMovementRecovering")
);

const FName
UBTService_BaruMovementRecovery::MovementRecoveryLocationKeyName(
    TEXT("MovementRecoveryLocation")
);


UBTService_BaruMovementRecovery::UBTService_BaruMovementRecovery()
{
    NodeName = TEXT("Baru Movement Recovery");

    // 몬스터마다 마지막 위치와 정지 시간을 따로 보관
    bCreateNodeInstance = true;

    // TickNode가 실행되도록 설정
    bNotifyTick = true;

    // 모든 몬스터가 같은 프레임에 검사하지 않도록 약간 분산
    Interval = 0.25f;
    RandomDeviation = 0.05f;
}


void UBTService_BaruMovementRecovery::TickNode(
    UBehaviorTreeComponent& OwnerComp,
    uint8* NodeMemory,
    float DeltaSeconds
)
{
    Super::TickNode(
        OwnerComp,
        NodeMemory,
        DeltaSeconds
    );

    AAIController* AIController =
        OwnerComp.GetAIOwner();

    UBlackboardComponent* BlackboardComponent =
        OwnerComp.GetBlackboardComponent();

    if (!IsValid(AIController) ||
        !AIController->HasAuthority() ||
        !IsValid(BlackboardComponent))
    {
        bHasLastObservedLocation = false;
        StationaryElapsedTime = 0.0f;
        RecoveryElapsedTime = 0.0f;
        return;
    }

    APawn* ControlledPawn =
        AIController->GetPawn();

    if (!IsValid(ControlledPawn))
    {
        bHasLastObservedLocation = false;
        StationaryElapsedTime = 0.0f;
        RecoveryElapsedTime = 0.0f;
        return;
    }

    const FVector CurrentLocation =
        ControlledPawn->GetActorLocation();

    UPathFollowingComponent* PathFollowingComponent =
        AIController->GetPathFollowingComponent();

    const bool bIsRecovering =
        BlackboardComponent->GetValueAsBool(
            IsMovementRecoveringKeyName
        );

    // =====================================================
    // 현재 복구 이동 중
    // =====================================================

    if (bIsRecovering)
    {
        RecoveryElapsedTime += DeltaSeconds;

        const FVector RecoveryLocation =
            BlackboardComponent->GetValueAsVector(
                MovementRecoveryLocationKeyName
            );

        const float DistanceSquared =
            (
                RecoveryLocation -
                CurrentLocation
            ).SizeSquared2D();

        const bool bReachedRecoveryLocation =
            DistanceSquared <=
            FMath::Square(RecoveryCompletionRadius);

        // 복구 분기가 시작될 시간을 조금 준 뒤
        // 경로 이동이 끝났는지 확인
        const bool bRecoveryMoveFinished =
            RecoveryElapsedTime >= 0.5f &&
            (
                !IsValid(PathFollowingComponent) ||
                PathFollowingComponent->GetStatus() !=
                EPathFollowingStatus::Moving
            );

        const bool bRecoveryTimedOut =
            RecoveryElapsedTime >= RecoveryTimeout;

        if (bReachedRecoveryLocation ||
            bRecoveryMoveFinished ||
            bRecoveryTimedOut)
        {
            if (bRecoveryTimedOut)
            {
                BARU_NET_LOG(
                    AIController,
                    LogBaruAI,
                    Warning,
                    TEXT(
                        "Movement recovery timed out. "
                        "Returning to normal behavior."
                    )
                );
            }

            FinishMovementRecovery(
                *BlackboardComponent,
                CurrentLocation
            );
        }

        return;
    }

    RecoveryElapsedTime = 0.0f;

    // =====================================================
    // 실제 Move To가 실행 중일 때만 막힘 검사
    // 대기·공격·수색 종료 상태는 막힘으로 판단하지 않음
    // =====================================================

    if (!IsValid(PathFollowingComponent) ||
        PathFollowingComponent->GetStatus() !=
        EPathFollowingStatus::Moving)
    {
        ResetMovementObservation(CurrentLocation);
        return;
    }

    // 첫 검사에서는 비교할 이전 위치만 저장
    if (!bHasLastObservedLocation)
    {
        ResetMovementObservation(CurrentLocation);
        return;
    }

    const float DistanceMoved =
        (
            CurrentLocation -
            LastObservedLocation
        ).Size2D();

    LastObservedLocation = CurrentLocation;

    // 정상적으로 움직이고 있다면 정지 시간 초기화
    if (DistanceMoved >= MinimumMovementDistance)
    {
        StationaryElapsedTime = 0.0f;
        return;
    }

    StationaryElapsedTime += DeltaSeconds;

    if (StationaryElapsedTime <
        StuckDetectionDuration)
    {
        return;
    }

    // =====================================================
    // 일정 시간 제자리걸음:
    // 좌우 우회 지점부터 탐색
    // =====================================================

    FVector RecoveryLocation;

    if (!FindRecoveryLocation(
            *ControlledPawn,
            RecoveryLocation
        ))
    {
        BARU_NET_LOG(
            AIController,
            LogBaruAI,
            Warning,
            TEXT(
                "Monster appears stuck, but no "
                "reachable recovery location was found."
            )
        );

        // 매 Tick마다 검색하지 않도록 다시 시간을 누적
        StationaryElapsedTime = 0.0f;
        return;
    }

    BeginMovementRecovery(
        *BlackboardComponent,
        RecoveryLocation,
        CurrentLocation
    );

    BARU_NET_LOG(
        AIController,
        LogBaruAI,
        Warning,
        TEXT(
            "Movement recovery started. "
            "Pawn: %s / Recovery location: %s"
        ),
        *GetNameSafe(ControlledPawn),
        *RecoveryLocation.ToString()
    );
}


bool UBTService_BaruMovementRecovery::FindRecoveryLocation(
    const APawn& ControlledPawn,
    FVector& OutRecoveryLocation
) const
{
    UWorld* World =
        ControlledPawn.GetWorld();

    AAIController* AIController =
        Cast<AAIController>(
            ControlledPawn.GetController()
        );

    if (!IsValid(World) ||
        !IsValid(AIController))
    {
        return false;
    }

    UNavigationSystemV1* NavigationSystem =
        FNavigationSystem::GetCurrent<
            UNavigationSystemV1
        >(World);

    if (!IsValid(NavigationSystem))
    {
        return false;
    }

    const FVector CurrentLocation =
        ControlledPawn.GetActorLocation();

    // 현재 진행 중인 경로의 다음 목표 방향 사용
    FVector MoveDirection =
        ControlledPawn
        .GetActorForwardVector()
        .GetSafeNormal2D();

    UPathFollowingComponent* PathFollowingComponent =
        AIController->GetPathFollowingComponent();

    if (IsValid(PathFollowingComponent) &&
        PathFollowingComponent->GetStatus() ==
        EPathFollowingStatus::Moving)
    {
        const FVector CurrentMoveTarget =
            PathFollowingComponent
            ->GetCurrentTargetLocation();

        const FVector DirectionToMoveTarget =
            (
                CurrentMoveTarget -
                CurrentLocation
            ).GetSafeNormal2D();

        if (!DirectionToMoveTarget.IsNearlyZero())
        {
            MoveDirection = DirectionToMoveTarget;
        }
    }

    if (MoveDirection.IsNearlyZero())
    {
        MoveDirection = FVector::ForwardVector;
    }

    const FVector RightDirection =
        FVector::CrossProduct(
            FVector::UpVector,
            MoveDirection
        ).GetSafeNormal();

    // 여러 몬스터가 모두 같은 방향으로 우회하지 않도록
    // 개체마다 좌우 우선순위를 나눔
    const bool bTryRightFirst =
        (ControlledPawn.GetUniqueID() % 2) == 0;

    const FVector FirstSideDirection =
        bTryRightFirst
        ? RightDirection
        : -RightDirection;

    const FVector SecondSideDirection =
        -FirstSideDirection;

    TArray<FVector> CandidateDirections;
    CandidateDirections.Reserve(5);

    // 1. 첫 번째 옆 방향
    CandidateDirections.Add(
        FirstSideDirection
    );

    // 2. 반대쪽 옆 방향
    CandidateDirections.Add(
        SecondSideDirection
    );

    // 3. 첫 번째 방향의 대각선 뒤
    CandidateDirections.Add(
        (
            FirstSideDirection -
            MoveDirection * 0.5f
        ).GetSafeNormal()
    );

    // 4. 반대쪽 방향의 대각선 뒤
    CandidateDirections.Add(
        (
            SecondSideDirection -
            MoveDirection * 0.5f
        ).GetSafeNormal()
    );

    // 5. 완전히 뒤로 후퇴
    CandidateDirections.Add(
        -MoveDirection
    );

    // 좁은 장소도 처리할 수 있도록
    // 긴 거리와 짧은 거리를 차례로 검사
    const float DistanceScales[] =
    {
        1.0f,
        0.65f
    };

    bool bFoundDirectedLocation = false;
    double BestPathLength = 0.0;

    for (const float DistanceScale :
         DistanceScales)
    {
        for (const FVector& CandidateDirection :
             CandidateDirections)
        {
            const FVector RawCandidateLocation =
                CurrentLocation +
                CandidateDirection *
                RecoverySearchRadius *
                DistanceScale;

            FNavLocation ProjectedNavLocation;

            // 계산한 위치를 실제 NavMesh 위로 보정
            const bool bProjectedToNavigation =
                NavigationSystem
                ->ProjectPointToNavigation(
                    RawCandidateLocation,
                    ProjectedNavLocation,
                    FVector(
                        100.0f,
                        100.0f,
                        200.0f
                    )
                );

            if (!bProjectedToNavigation)
            {
                continue;
            }

            const FVector CandidateLocation =
                ProjectedNavLocation.Location;

            // 현재 위치와 사실상 같은 지점은 제외
            if ((
                    CandidateLocation -
                    CurrentLocation
                ).SizeSquared2D() <
                FMath::Square(
                    MinimumRecoveryDistance
                ))
            {
                continue;
            }

            // 현재 위치에서 후보 지점까지
            // 실제 이동 가능한 경로인지 확인
            UNavigationPath* CandidatePath =
                UNavigationSystemV1
                ::FindPathToLocationSynchronously(
                    World,
                    CurrentLocation,
                    CandidateLocation,
                    AIController,
                    nullptr
                );

            if (!IsValid(CandidatePath) ||
                !CandidatePath->IsValid() ||
                CandidatePath->IsPartial() ||
                CandidatePath->PathPoints.Num() < 2)
            {
                continue;
            }

            const double CandidatePathLength =
                CandidatePath->GetPathLength();

            // 복구인데 지나치게 멀리 돌아가는 지점은 제외
            if (CandidatePathLength <= 0.0 ||
                CandidatePathLength >
                static_cast<double>(
                    RecoverySearchRadius
                ) * 3.0)
            {
                continue;
            }

            // 좌우 후보 중 실제 경로가 가장 짧은 위치 선택
            if (!bFoundDirectedLocation ||
                CandidatePathLength <
                BestPathLength)
            {
                BestPathLength =
                    CandidatePathLength;

                OutRecoveryLocation =
                    CandidateLocation;

                bFoundDirectedLocation = true;
            }
        }
    }

    if (bFoundDirectedLocation)
    {
        return true;
    }

    // =====================================================
    // 좌우·뒤쪽 후보가 모두 실패했을 때만
    // 주변의 임의 도달 가능 지점을 최후 수단으로 사용
    // =====================================================

    for (int32 AttemptIndex = 0;
         AttemptIndex < RecoveryPointSearchAttempts;
         ++AttemptIndex)
    {
        FNavLocation RandomNavLocation;

        if (!NavigationSystem
            ->GetRandomReachablePointInRadius(
                CurrentLocation,
                RecoverySearchRadius,
                RandomNavLocation
            ))
        {
            continue;
        }

        const FVector RandomLocation =
            RandomNavLocation.Location;

        if ((
                RandomLocation -
                CurrentLocation
            ).SizeSquared2D() <
            FMath::Square(
                MinimumRecoveryDistance
            ))
        {
            continue;
        }

        UNavigationPath* RandomPath =
            UNavigationSystemV1
            ::FindPathToLocationSynchronously(
                World,
                CurrentLocation,
                RandomLocation,
                AIController,
                nullptr
            );

        if (!IsValid(RandomPath) ||
            !RandomPath->IsValid() ||
            RandomPath->IsPartial())
        {
            continue;
        }

        OutRecoveryLocation = RandomLocation;
        return true;
    }

    return false;
}


void UBTService_BaruMovementRecovery::
ResetMovementObservation(
    const FVector& CurrentLocation
)
{
    LastObservedLocation = CurrentLocation;
    StationaryElapsedTime = 0.0f;
    bHasLastObservedLocation = true;
}


void UBTService_BaruMovementRecovery::
BeginMovementRecovery(
    UBlackboardComponent& BlackboardComponent,
    const FVector& RecoveryLocation,
    const FVector& CurrentLocation
)
{
    // 위치를 먼저 기록한 다음 Bool을 켜야
    // BT가 복구 분기로 전환될 때 목적지가 이미 존재함
    BlackboardComponent.SetValueAsVector(
        MovementRecoveryLocationKeyName,
        RecoveryLocation
    );

    BlackboardComponent.SetValueAsBool(
        IsMovementRecoveringKeyName,
        true
    );

    LastObservedLocation = CurrentLocation;
    StationaryElapsedTime = 0.0f;
    RecoveryElapsedTime = 0.0f;
    bHasLastObservedLocation = true;
}


void UBTService_BaruMovementRecovery::
FinishMovementRecovery(
    UBlackboardComponent& BlackboardComponent,
    const FVector& CurrentLocation
)
{
    // Bool을 먼저 끄면 최우선 복구 분기가 종료되고
    // 원래 추적·수색 행동으로 돌아감
    BlackboardComponent.SetValueAsBool(
        IsMovementRecoveringKeyName,
        false
    );

    BlackboardComponent.ClearValue(
        MovementRecoveryLocationKeyName
    );

    RecoveryElapsedTime = 0.0f;

    ResetMovementObservation(CurrentLocation);
}
