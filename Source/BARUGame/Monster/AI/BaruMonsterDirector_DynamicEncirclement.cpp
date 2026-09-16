#include "Monster/AI/BaruMonsterDirector.h"

#include "Monster/AI/BaruMonsterAIController.h"
#include "Monster/Characters/BaruMonsterCharacter.h"
#include "Player/BaruPlayerState.h"
#include "Interfaces/CombatInterface.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Monster/Data/BaruMonsterDataAsset.h"
#include "DrawDebugHelpers.h"
#include "Templates/UnrealTemplate.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Engine/World.h"
#include "BaruLog.h"

namespace BaruDynamicEncirclement
{
    bool IsLivingPlayer(const APawn* Player)
    {
        const ABaruPlayerState* State = IsValid(Player)
            ? Player->GetPlayerState<ABaruPlayerState>() : nullptr;
        return IsValid(Player) && Player->IsPlayerControlled() &&
            IsValid(State) && State->IsAlive();
    }

    bool IsRecovering(const ABaruMonsterAIController* Controller)
    {
        const UBlackboardComponent* Blackboard = Controller->GetBlackboardComponent();
        return IsValid(Blackboard) &&
            Blackboard->GetValueAsBool(TEXT("IsMovementRecovering"));
    }

    bool IsCompletePath(UNavigationPath* Path)
    {
        return IsValid(Path) && Path->IsValid() && !Path->IsPartial() &&
            Path->PathPoints.Num() >= 2 && FMath::IsFinite(Path->GetPathLength());
    }

    // 경로의 누적 거리를 따라 차단 후보를 선택
    FVector PointAlongPath(const TArray<FVector>& Points, double Distance)
    {
        for (int32 Index = 1; Index < Points.Num(); ++Index)
        {
            const double Length = FVector::Distance(Points[Index - 1], Points[Index]);
            if (Length > UE_SMALL_NUMBER && Distance <= Length)
            {
                return FMath::Lerp(Points[Index - 1], Points[Index], Distance / Length);
            }
            Distance -= Length;
        }
        return Points.Last();
    }

    // 플레이어가 기존 차단 지점을 지나쳤거나 경로를 벗어났는지 검사
    double ClosestPathProgress(const TArray<FVector>& Points, const FVector& Location,
        double& OutDistanceSquared)
    {
        double Progress = 0.0;
        double BestProgress = 0.0;
        OutDistanceSquared = TNumericLimits<double>::Max();
        for (int32 Index = 1; Index < Points.Num(); ++Index)
        {
            const FVector Segment = Points[Index] - Points[Index - 1];
            const double LengthSquared = Segment.SizeSquared();
            const double Alpha = LengthSquared > UE_SMALL_NUMBER
                ? FMath::Clamp(FVector::DotProduct(Location - Points[Index - 1], Segment) /
                    LengthSquared, 0.0, 1.0) : 0.0;
            const double DistanceSquared = FVector::DistSquared(
                Location, Points[Index - 1] + Segment * Alpha);
            const double Length = FMath::Sqrt(LengthSquared);
            if (DistanceSquared < OutDistanceSquared)
            {
                OutDistanceSquared = DistanceSquared;
                BestProgress = Progress + Length * Alpha;
            }
            Progress += Length;
        }
        return BestProgress;
    }

    // 우회 경로가 플레이어 바로 앞을 다시 통과하는 후보는 제외
    bool KeepsDistanceFromTarget(const UNavigationPath& Path, const FVector& Target)
    {
        FVector FlatTarget = Target;
        FlatTarget.Z = 0.0;
        for (int32 Index = 1; Index < Path.PathPoints.Num(); ++Index)
        {
            FVector Start = Path.PathPoints[Index - 1];
            FVector End = Path.PathPoints[Index];
            Start.Z = End.Z = 0.0;
            const FVector Segment = End - Start;
            const double LengthSquared = Segment.SizeSquared();
            const double Alpha = LengthSquared > UE_SMALL_NUMBER
                ? FMath::Clamp(FVector::DotProduct(FlatTarget - Start, Segment) /
                    LengthSquared, 0.0, 1.0) : 0.0;
            if (FVector::DistSquared(FlatTarget, Start + Segment * Alpha) <
                FMath::Square(200.0))
            {
                return false;
            }
        }
        return true;
    }

    // NavMesh 위라도 현재 벽·장애물과 캡슐이 겹치는 목적지는 제외
    bool HasRoomAt(const ABaruMonsterCharacter& Monster, const FVector& FeetLocation)
    {
        const UCapsuleComponent* Capsule = Monster.GetCapsuleComponent();
        UWorld* World = Monster.GetWorld();
        if (!IsValid(Capsule) || !IsValid(World))
        {
            return false;
        }
        const float Radius = Capsule->GetScaledCapsuleRadius();
        const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
        FCollisionQueryParams Params(SCENE_QUERY_STAT(BaruEncirclementRoom), false, &Monster);
        return !World->OverlapBlockingTestByChannel(
            FeetLocation + FVector(0.0, 0.0, HalfHeight + 2.0),
            FQuat::Identity,
            Capsule->GetCollisionObjectType(),
            FCollisionShape::MakeCapsule(Radius, HalfHeight),
            Params,
            FCollisionResponseParams(Capsule->GetCollisionResponseToChannels())
        );
    }
    
     double RouteLength(const TArray<FVector>& Points)
    {
        double Length = 0.0;

        for (int32 Index = 1; Index < Points.Num(); ++Index)
        {
            Length += FVector::Distance(
                Points[Index - 1],
                Points[Index]
            );
        }

        return Length;
    }

    bool FindSeparateApproach(
    const TArray<FVector>& Path,
    const TArray<FVector>& Reference,
    FVector& OutWaypoint)
    {
        if (Path.Num() < 2 || Reference.Num() < 2)
        {
            return false;
        }

        const double Length = RouteLength(Path);
        const double Span = FMath::Min(2200.0, Length - 150.0);
        int32 SeparateSamples = 0;

        for (double Back = 300.0; Back <= Span; Back += 150.0)
        {
            const FVector Point =
                PointAlongPath(Path, Length - Back);

            double DistanceSquared = 0.0;
            const double Progress =
                ClosestPathProgress(Reference, Point, DistanceSquared);

            // 짧은 기준 경로의 뒤쪽 연장선을 다른 통로로 오인하지 않음
            if (Progress <= UE_SMALL_NUMBER)
            {
                const FVector Backward = Reference[0] - Reference[1];
                const double SizeSquared = Backward.SizeSquared();

                if (SizeSquared > UE_SMALL_NUMBER)
                {
                    const double Alpha = FMath::Max(
                        0.0,
                        FVector::DotProduct(Point - Reference[0], Backward)
                            / SizeSquared
                    );

                    DistanceSquared = FMath::Min(
                        DistanceSquared,
                        FVector::DistSquared(
                            Point,
                            Reference[0] + Backward * Alpha
                        )
                    );
                }
            }

            SeparateSamples =
                DistanceSquared >= FMath::Square(350.0)
                    ? SeparateSamples + 1
                    : 0;

            if (SeparateSamples >= 2)
            {
                OutWaypoint = Point;
                return true;
            }
        }

        return false;
    }
    
    // 차단 지점에 접근하는 마지막 구간의 좌우 방향을 구분
    int32 GetApproachSide(
        const TArray<FVector>& Path,
        const FVector& Forward)
    {
        const double Length = RouteLength(Path);
        const double Span = FMath::Min(900.0, Length - 150.0);
        const FVector Side(-Forward.Y, Forward.X, 0.0);

        int32 PreviousSide = 0;
        int32 Consecutive = 0;

        for (double Back = 300.0; Back <= Span; Back += 150.0)
        {
            const FVector Offset =
                PointAlongPath(Path, Length - Back) - Path.Last();

            const double Lateral = FVector::DotProduct(Offset, Side);
            const int32 CurrentSide =
                Lateral >= 250.0 ? 1 : (Lateral <= -250.0 ? -1 : 0);

            Consecutive =
                CurrentSide != 0 && CurrentSide == PreviousSide
                    ? Consecutive + 1
                    : (CurrentSide != 0 ? 1 : 0);

            PreviousSide = CurrentSide;

            if (Consecutive >= 2)
            {
                return CurrentSide;
            }
        }

        return 0;
    }

    // 막다른 곳에 들어갔다가 같은 길로 돌아오는 가짜 우회는 제외
    bool RetracesRoute(
        const TArray<FVector>& First,
        const TArray<FVector>& Second
    )
    {
        const double Length = RouteLength(First);
        int32 OverlapSamples = 0;

        for (
            double Distance = 0.0;
            Distance < Length - 300.0;
            Distance += 150.0
        )
        {
            double DistanceSquared = 0.0;

            ClosestPathProgress(
                Second,
                PointAlongPath(First, Distance),
                DistanceSquared
            );

            if (DistanceSquared < FMath::Square(100.0) &&
                ++OverlapSamples >= 2)
            {
                return true;
            }
        }

        return false;
    }
    
}

void ABaruMonsterDirector::UpdateExtractionContext(
    AActor* SourceElevator,
    APawn* TargetPlayer,
    const FVector& BoardingLocation,
    float SecondsRemaining)
{
    if (GetNetMode() == NM_Client || !IsValid(GetWorld()) ||
        !IsValid(SourceElevator) || SourceElevator->GetWorld() != GetWorld())
    {
        return;
    }

    if (!BaruDynamicEncirclement::IsLivingPlayer(TargetPlayer) ||
        TargetPlayer->GetWorld() != GetWorld() || BoardingLocation.ContainsNaN() ||
        !FMath::IsFinite(SecondsRemaining) || SecondsRemaining <= 0.0f)
    {
        ClearExtractionContext(SourceElevator);
        return;
    }

    // 동시에 다른 엘리베이터가 보내는 요청으로 목표를 덮어쓰지 않음
    if (HasValidExtractionContext() && ExtractionSourceElevator.Get() != SourceElevator)
    {
        return;
    }

    const bool bChanged = !bHasExtractionContext ||
        ExtractionSourceElevator.Get() != SourceElevator ||
        ExtractionTargetPlayer.Get() != TargetPlayer ||
        !ExtractionBoardingLocation.Equals(BoardingLocation, 100.0);

    ExtractionSourceElevator = SourceElevator;
    ExtractionBoardingLocation = BoardingLocation;
    LastExtractionContextTime = GetWorld()->GetTimeSeconds();
    ExtractionDeadline = LastExtractionContextTime + SecondsRemaining;
    bHasExtractionContext = true;

    if (ExtractionTargetPlayer.Get() != TargetPlayer)
    {
        SetExtractionTarget(TargetPlayer);
    }
    SetExtractionActive(true);

    if (bChanged)
    {
        if (bHasDynamicEncirclement &&
            DynamicAssignmentState == EBaruDirectorState::Extraction)
        {
            FinishDynamicEncirclement(TEXT("Extraction context changed"));
        }
        BARU_NET_LOG(this, LogBaruAI, Log,
            TEXT("Extraction context: Target=%s, Boarding=%s, Remaining=%.1f"),
            *GetNameSafe(TargetPlayer), *BoardingLocation.ToCompactString(), SecondsRemaining);
    }
}

void ABaruMonsterDirector::ClearExtractionContext(AActor* SourceElevator)
{
    if (GetNetMode() == NM_Client || !bHasExtractionContext ||
        ExtractionSourceElevator.Get() != SourceElevator)
    {
        return;
    }

    bHasExtractionContext = false;
    ExtractionSourceElevator.Reset();
    ExtractionDeadline = -1.0;
    LastExtractionContextTime = -1.0;
    SetExtractionActive(false);
    SetExtractionTarget(nullptr);
    RefreshEncirclementAssignment();
    
    // 취소·전원 탑승·출발 시 집결 명령 해제
    RefreshExtractionRally();
}

bool ABaruMonsterDirector::HasValidExtractionContext() const
{
    if (!bHasExtractionContext || !bExtractionActive ||
        !ExtractionSourceElevator.IsValid() || !IsValid(GetWorld()) ||
        !BaruDynamicEncirclement::IsLivingPlayer(ExtractionTargetPlayer.Get()))
    {
        return false;
    }
    const double Now = GetWorld()->GetTimeSeconds();
    return Now < ExtractionDeadline && Now - LastExtractionContextTime <= 2.5;
}

void ABaruMonsterDirector::RefreshTacticalState()
{
    if (GetNetMode() == NM_Client)
    {
        return;
    }

    if (bHasExtractionContext && !HasValidExtractionContext())
    {
        ClearExtractionContext(ExtractionSourceElevator.Get());
    }

    // 기존 포위의 종료만 검사하고 새 포위는 웨이브 생성 때 배정
    RefreshEncirclementAssignment();
    RefreshExtractionRally();
}

APawn* ABaruMonsterDirector::FindDynamicPressureTarget() const
{
    APawn* BestTarget = nullptr;
    float BestThreat = -1.0f;
    double BestDistanceSquared = TNumericLimits<double>::Max();

    for (const TWeakObjectPtr<ABaruMonsterCharacter>& Entry : RegisteredMonsters)
    {
        ABaruMonsterCharacter* Monster = Entry.Get();
        if (!IsValid(Monster) || ICombatInterface::Execute_IsDead(Monster))
        {
            continue;
        }
        const ABaruMonsterAIController* Controller =
            Cast<ABaruMonsterAIController>(Monster->GetController());
        if (!IsValid(Controller) || !Controller->HasAuthority() ||
            BaruDynamicEncirclement::IsRecovering(Controller))
        {
            continue;
        }
        const EBaruMonsterDirectorCommand Command = Controller->GetDirectorCommand();
        if (Command != EBaruMonsterDirectorCommand::None &&
            Command != EBaruMonsterDirectorCommand::Hold)
        {
            continue;
        }
        APawn* Target = Controller->GetCurrentTarget();
        if (!BaruDynamicEncirclement::IsLivingPlayer(Target))
        {
            continue;
        }
        const float Threat = GetPlayerThreat(Target);
        const double DistanceSquared = FVector::DistSquared(
            Monster->GetActorLocation(), Target->GetActorLocation());
        if (Threat > BestThreat || (Threat == BestThreat && DistanceSquared < BestDistanceSquared))
        {
            BestTarget = Target;
            BestThreat = Threat;
            BestDistanceSquared = DistanceSquared;
        }
    }
    return BestTarget;
}

bool ABaruMonsterDirector::TryAssignDynamicEncirclement(
    APawn* TargetPlayer,
    const TArray<TWeakObjectPtr<ABaruMonsterCharacter>>& NewWaveMonsters,
    int32 RequestedBlockers
)
{
    using namespace BaruDynamicEncirclement;

    UWorld* World = GetWorld();

    if (GetNetMode() == NM_Client ||
        !IsValid(World) ||
        !bUseDynamicEncirclement ||
        RequestedBlockers <= 0 ||
        NewWaveMonsters.IsEmpty() ||
        !IsEncirclementAllowed() ||
        !IsLivingPlayer(TargetPlayer) ||
        bUpdatingDynamicGroup ||
        CurrentEncirclementBlocker.IsValid())
    {
        return false;
    }

    const bool bExtraction =
        CurrentDirectorState == EBaruDirectorState::Extraction;

    if (bExtraction &&
        (!HasValidExtractionContext() ||
         ExtractionTargetPlayer.Get() != TargetPlayer))
    {
        return false;
    }

    const double Now = World->GetTimeSeconds();

    TGuardValue<bool> UpdateGuard(bUpdatingDynamicGroup, true);

    // 이번 웨이브에서만 추가 배정하며 이후 부족분을 보충하지 않음
    LastEncirclementDecisionTime = Now;
    RemoveInvalidMonsters();
    RequestedBlockers = FMath::Min(RequestedBlockers, NewWaveMonsters.Num() / 2);
    if (RequestedBlockers <= 0)
    {
        return false;
    }
    const int32 PreviousBlockers = DynamicAssignments.Num();

    if (!DynamicAssignments.IsEmpty() &&
        (DynamicAssignmentState != CurrentDirectorState ||
         DynamicAssignments[0].Target.Get() != TargetPlayer))
    {
        return false;
    }

    // 스포너가 방금 내린 조사 명령도 포위 명령으로 전환 가능
    auto IsWaveInvestigating = [&NewWaveMonsters, TargetPlayer](
        ABaruMonsterCharacter* Monster, ABaruMonsterAIController* Controller)
    {
        APawn* CurrentTarget = Controller->GetCurrentTarget();
        return NewWaveMonsters.Contains(TWeakObjectPtr<ABaruMonsterCharacter>(Monster)) &&
            Controller->GetDirectorCommand() == EBaruMonsterDirectorCommand::Investigate &&
            (!IsValid(CurrentTarget) || CurrentTarget == TargetPlayer) &&
            (!Controller->HasLastKnownTargetLocation() || CurrentTarget == TargetPlayer);
    };

    UNavigationSystemV1* Nav =
        FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);

    if (!IsValid(Nav))
    {
        return false;
    }

    // 전체 경로 탐색 횟수와 현재 차단 후보에 허용된 탐색 한도
    int32 Queries = 0;
    int32 QueryLimit = 64;
    
    // 경로가 어느 검사 단계에서 탈락하는지 집계
    int32 SeparatedRoutes = 0;
    int32 ClearRoutes = 0;

    // 도착 시간 때문에 제외된 이유를 구분
    int32 LifetimeRejected = 0;
    int32 DeadlineRejected = 0;
    int32 PlayerArrivalRejected = 0;

    auto FindPath = [&](
        const FVector& Start,
        const FVector& End,
        AActor* Context,
        TArray<FVector>& OutPoints
    ) -> bool
    {
        OutPoints.Reset();

        if (Queries >= QueryLimit)
        {
            return false;
        }

        ++Queries;

        UNavigationPath* Path =
            UNavigationSystemV1::FindPathToLocationSynchronously(
                World,
                Start,
                End,
                Context,
                nullptr
            );

        if (!IsCompletePath(Path))
        {
            return false;
        }

        OutPoints = Path->PathPoints;
        return true;
    };

    const FVector TargetStart =
        TargetPlayer->GetNavAgentLocation();

    const double PlayerSpeed =
        TargetPlayer->GetVelocity().Size2D();

    // 이동 중이면 속도 방향을 사용하고, 정지 중이면 바라보는 방향 사용
    FVector Heading = PlayerSpeed >= 100.0
        ? TargetPlayer->GetVelocity().GetSafeNormal2D()
        : TargetPlayer->GetActorForwardVector().GetSafeNormal2D();

    if (Heading.IsNearlyZero())
    {
        Heading = FVector::ForwardVector;
    }

    TArray<FVector> PlayerPath;

int32 ProjectionFailed = 0;
int32 PathFailed = 0;
int32 TooShort = 0;
int32 WrongDirection = 0;
double LastLength = -1.0;

// 탈출 상태에서는 엘리베이터 근처의 짧은 경로도 검사
const double MinimumPathLength = bExtraction ? 450.0 : 700.0;

const double Prediction = FMath::Clamp(
    FMath::Max(250.0, PlayerSpeed)
        * FMath::Max(0.5f, DynamicPredictionTime),
    1000.0,
    4000.0
);

const double Angles[] = { 0.0, 45.0, -45.0, 90.0, -90.0, 0.0 };

for (int32 Attempt = 0; Attempt < (bExtraction ? 1 : 6); ++Attempt)
{
    const FVector Goal =
        bExtraction
            ? ExtractionBoardingLocation
            : TargetStart
                + Heading.RotateAngleAxis(
                    Angles[Attempt],
                    FVector::UpVector
                ) * Prediction * (Attempt == 5 ? 0.5 : 1.0);

    FNavLocation Projected;

    if (!Nav->ProjectPointToNavigation(
        Goal,
        Projected,
        FVector(180.0, 180.0, 150.0),
        &TargetPlayer->GetNavAgentPropertiesRef()))
    {
        ++ProjectionFailed;
        continue;
    }

    TArray<FVector> CandidatePath;

    if (!FindPath(
        TargetStart,
        Projected.Location,
        TargetPlayer,
        CandidatePath))
    {
        ++PathFailed;
        continue;
    }

    LastLength = RouteLength(CandidatePath);

    if (LastLength < MinimumPathLength)
    {
        ++TooShort;
        continue;
    }

    const FVector FirstDirection =
        (PointAlongPath(CandidatePath, 300.0) - TargetStart)
            .GetSafeNormal2D();

    if (!bExtraction &&
        FVector::DotProduct(FirstDirection, Heading) < 0.1)
    {
        ++WrongDirection;
        continue;
    }

    PlayerPath = MoveTemp(CandidatePath);
    break;
}

if (PlayerPath.IsEmpty())
{
    BARU_NET_LOG(
        this,
        LogBaruAI,
        Log,
        TEXT("Player route rejected: Mode=%s, Projection=%d, Path=%d, Short=%d, Direction=%d, Length=%.0f, Minimum=%.0f"),
        bExtraction ? TEXT("Extraction") : TEXT("Pressure"),
        ProjectionFailed,
        PathFailed,
        TooShort,
        WrongDirection,
        LastLength,
        MinimumPathLength
    );

    return false;
}

    // 가장 가까이서 추적하는 몬스터는 우회 후보에서 제외
    ABaruMonsterCharacter* PressureMonster = nullptr;
    double PressureDistance = TNumericLimits<double>::Max();
    
    // 플레이어를 기준으로 네 방향에 있는 후보들을 나눠 보관
    TArray<ABaruMonsterCharacter*> Buckets[4];
    
    // 같은 목표를 추적하거나 새 명령을 받을 수 있는 참여 개체 수
    int32 Participants = 0;

    const double SearchSquared = FMath::Square(
        static_cast<double>(
            FMath::Max(100.0f, MaximumEncirclementCandidateDistance)
        )
    );

    for (
        const TWeakObjectPtr<ABaruMonsterCharacter>& Entry :
        RegisteredMonsters
    )
    {
        ABaruMonsterCharacter* Monster = Entry.Get();

        if (!IsValid(Monster) ||
            ICombatInterface::Execute_IsDead(Monster))
        {
            continue;
        }

        ABaruMonsterAIController* Controller =
            Cast<ABaruMonsterAIController>(Monster->GetController());

        if (!IsValid(Controller) ||
            !Controller->HasAuthority() ||
            IsRecovering(Controller))
        {
            continue;
        }

        const EBaruMonsterDirectorCommand Command =
            Controller->GetDirectorCommand();

        APawn* CurrentTarget =
            Controller->GetCurrentTarget();

        // 탈출 집결 중인 몬스터도 우회 후보에 포함
        const bool bRallyCandidate =
            bExtraction &&
            Command ==
                EBaruMonsterDirectorCommand::ExtractionRally &&
            Controller->GetExtractionRallyTarget() ==
                TargetPlayer;

        const bool bPursuing =
            CurrentTarget == TargetPlayer &&
            (Command == EBaruMonsterDirectorCommand::None ||
             Command == EBaruMonsterDirectorCommand::Hold);

        const bool bIdle =
            Command == EBaruMonsterDirectorCommand::None &&
            !IsValid(CurrentTarget) &&
            !Controller->HasLastKnownTargetLocation();

        const bool bWaveInvestigating = IsWaveInvestigating(Monster, Controller);
        if (!bPursuing && !bIdle && !bRallyCandidate && !bWaveInvestigating)
        {
            continue;
        }

        const FVector Location =
            Monster->GetNavAgentLocation();

        const double Distance =
            FVector::DistSquared(Location, TargetStart);

        if (FMath::Min(
                Distance,
                FVector::DistSquared(Location, PlayerPath.Last())
            ) > SearchSquared)
        {
            continue;
        }

        ++Participants;

        if (bPursuing && Distance < PressureDistance)
        {
            PressureMonster = Monster;
            PressureDistance = Distance;
        }

        // 맵 배치 개체와 이전 웨이브는 이번 포위 후보에서 제외
        if (!NewWaveMonsters.Contains(Entry))
        {
            continue;
        }

        if ((Command != EBaruMonsterDirectorCommand::None &&
             !bRallyCandidate && !bWaveInvestigating) ||
            Distance < FMath::Square(300.0))
        {
            continue;
        }

        const FVector Offset = Location - TargetStart;

        const int32 Sector =
            (Offset.X >= 0.0 ? 1 : 0) +
            (Offset.Y >= 0.0 ? 2 : 0);

        Buckets[Sector].Add(Monster);
    }

    if ((!bExtraction &&
         (!IsValid(PressureMonster) ||
          Participants < FMath::Max(2, MinimumEncirclementParticipants))) ||
        Participants == 0)
    {
        BARU_NET_LOG(
            this, LogBaruAI, Log,
            TEXT("Group encirclement skipped: participants=%d, pressure=%s"),
            Participants,
            *GetNameSafe(PressureMonster)
        );

        return false;
    }

    for (auto& Bucket : Buckets)
    {
        Bucket.Remove(PressureMonster);

        Bucket.Sort([&](
            const ABaruMonsterCharacter& A,
            const ABaruMonsterCharacter& B
        )
        {
            return FVector::DistSquared(
                A.GetActorLocation(), PlayerPath.Last()
            ) < FVector::DistSquared(
                B.GetActorLocation(), PlayerPath.Last()
            );
        });
    }

    TArray<ABaruMonsterCharacter*> Candidates;

    const int32 SectorStart = DynamicCandidateCursor;
    DynamicCandidateCursor = (DynamicCandidateCursor + 1) % 4;

    const int32 CandidateLimit = FMath::Max(
        FMath::Clamp(MaximumDynamicCandidates, 1, 8),
        RequestedBlockers);

    // 한 방향의 가까운 몬스터만 후보 목록을 채우지 않도록 순환
    for (
        int32 Rank = 0;
        Rank < CandidateLimit && Candidates.Num() < CandidateLimit;
        ++Rank
    )
    {
        for (
            int32 SectorIndex = 0;
            SectorIndex < 4 && Candidates.Num() < CandidateLimit;
            ++SectorIndex
        )
        {
            const auto& Bucket =
                Buckets[(SectorStart + SectorIndex) % 4];

            if (Bucket.IsValidIndex(Rank))
            {
                Candidates.Add(Bucket[Rank]);
            }
        }
    }

    // 명령을 내리기 전에 비교할 몬스터·경로 조합
    struct FOption
        {
            ABaruMonsterCharacter* Monster = nullptr;

            // 먼저 방문할 경유지와 최종 차단 위치
            FVector Route = FVector::ZeroVector;
            FVector Block = FVector::ZeroVector;

            // 경유지를 포함한 몬스터의 예상 이동 경로
            TArray<FVector> Path;

            // 플레이어 경로 시작점부터 차단 위치까지의 거리
            double Progress = 0.0;

            // 몬스터의 예상 도착 시간과 후보 비교 점수
            double Arrival = 0.0;
            double Score = 0.0;
            int32 ApproachSide = 0;
        };

    TArray<FOption> Options;

    // 최종 목적지에서 250cm 앞까지를 차단 후보 범위로 사용
    const double Available =
    RouteLength(PlayerPath) - (bExtraction ? 100.0 : 250.0);

    // 플레이어의 예상 진행 방향을 기준으로 양쪽을 구분
    FVector ApproachForward =
        (PlayerPath.Last() - TargetStart).GetSafeNormal2D();

    if (ApproachForward.IsNearlyZero())
    {
        ApproachForward = Heading;
    }
    
    const int32 BlockSlotCount = FMath::Max(3, RequestedBlockers);

    for (int32 Pass = 0; Pass < 2 && Queries < 64; ++Pass)
    {
        const int32 PassLimit = Pass == 0 ? 32 : 64;
        for (int32 SlotIndex = 0;
             SlotIndex < BlockSlotCount && Queries < PassLimit; ++SlotIndex)
        {
            // 한 번의 계획 안에서 후보를 검사하며 총 경로 탐색은 64회 제한
            QueryLimit = FMath::Min(PassLimit, Queries + FMath::Max(
                1, (PassLimit - Queries) / (BlockSlotCount - SlotIndex)));
            const double Alpha = static_cast<double>(SlotIndex) / (BlockSlotCount - 1);
            const double Progress = FMath::Lerp(
                Available, bExtraction ? 300.0 : 450.0, Alpha);

            if (Progress < (bExtraction ? 300.0 : 450.0) || Queries >= 64)
            {
                continue;
            }

            const FVector RawBlock =
                PointAlongPath(PlayerPath, Progress);

            TArray<FVector> Reference;

            // 플레이어의 진행 경로를 기준으로 우회 여부를 비교
            if (!FindPath(TargetStart, RawBlock, TargetPlayer, Reference))
            {
                continue;
            }

            for (
                int32 CandidateIndex = 0;
                CandidateIndex < Candidates.Num() && Queries < QueryLimit;
                ++CandidateIndex
            )
            {
                ABaruMonsterCharacter* Monster = Candidates[
                    (CandidateIndex + SlotIndex) %
                    Candidates.Num()
                ];

                ABaruMonsterAIController* Controller =
                    Cast<ABaruMonsterAIController>(Monster->GetController());

                UCharacterMovementComponent* Movement =
                    Monster->GetCharacterMovement();

                if (!IsValid(Controller) ||
                    !IsValid(Movement) ||
                    !Movement->IsMovingOnGround())
                {
                    continue;
                }

                const UBaruMonsterDataAsset* Data =
                    Monster->GetMonsterDataAsset();

                // BT에서 사용할 추적 속도를 기준으로 도착 시간 계산
                const double Speed = FMath::Max(
                    100.0,
                    static_cast<double>(
                        IsValid(Data)
                            ? Data->ChaseSpeed
                            : Movement->MaxWalkSpeed
                    )
                );

                const FVector Start = Monster->GetNavAgentLocation();

                FNavLocation ProjectedBlock;

                if (!Nav->ProjectPointToNavigation(
                        RawBlock,
                        ProjectedBlock,
                        FVector(60.0, 60.0, 100.0),
                        &Monster->GetNavAgentPropertiesRef()) ||
                    !HasRoomAt(*Monster, ProjectedBlock.Location))
                {
                    continue;
                }

                const FVector Block = ProjectedBlock.Location;

                if (FVector::Distance(Block, TargetStart) < 300.0)
                {
                    continue;
                }

                const int32 RouteAttempts = Pass == 0 ? 1 : 4;

                for (
                    int32 Attempt = 0;
                    Attempt < RouteAttempts && Queries < QueryLimit;
                    ++Attempt
                )
                {
                    FVector Route = FVector::ZeroVector;
                    TArray<FVector> Path;

                    if (Pass == 0)
                    {
                        if (!FindPath(Start, Block, Controller, Path) ||
                            !FindSeparateApproach(Path, Reference, Route))
                        {
                            continue;
                        }
                    }
                    else
                    {
                        const FVector Forward =
                            (Block - TargetStart).GetSafeNormal2D();

                        const FVector Side(-Forward.Y, Forward.X, 0.0);

                        const double Offset =
                            FMath::Max(600.0f, DynamicFlankOffset) *
                            (Attempt < 2 ? 1.0 : 2.0) *
                            (Attempt % 2 == 0 ? -1.0 : 1.0);

                        FNavLocation ProjectedRoute;

                        if (!Nav->ProjectPointToNavigation(
                                (TargetStart + Block) * 0.5 + Side * Offset,
                                ProjectedRoute,
                                FVector(200.0, 200.0, 150.0),
                                &Monster->GetNavAgentPropertiesRef()))
                        {
                            continue;
                        }

                        Route = ProjectedRoute.Location;
                        TArray<FVector> ToBlock;

                        if (!FindPath(Start, Route, Controller, Path) ||
                            !FindPath(Route, Block, Controller, ToBlock) ||
                            RetracesRoute(Path, ToBlock))
                        {
                            continue;
                        }

                        Path.Append(ToBlock);

                        FVector SeparatePoint;

                        if (!FindSeparateApproach(
                                Path,
                                Reference,
                                SeparatePoint))
                        {
                            continue;
                        }
                    }

                    // 추적 경로와 다른 접근 구간을 찾은 후보 수
                    ++SeparatedRoutes;
                    
                    if (!HasRoomAt(*Monster, Route) ||
                        FVector::Distance(Start, Route) < 150.0 ||
                        FVector::Distance(Route, Block) < 200.0)
                    {
                        continue;
                    }

                    double DistanceFromTarget = 0.0;

                    ClosestPathProgress(
                        Path,
                        TargetStart,
                        DistanceFromTarget
                    );

                    if (DistanceFromTarget < FMath::Square(250.0))
                    {
                        continue;
                    }
                    
                    // 목적지 공간과 플레이어 근접 회피 검사까지 통과
                    ++ClearRoutes;

                    const double Length = RouteLength(Path);
                    const double Arrival = Length / Speed + 0.75;

                    // 이동과 5초 대기가 계획 제한 시간을 넘는 경우
                    if (Arrival + 5.0 >
                        FMath::Max(6.0f, DynamicPlanLifetime))
                    {
                        ++LifetimeRejected;
                        continue;
                    }

                    // 엘리베이터 출발 전에 도착하기 어려운 경우
                    if (bExtraction &&
                        Arrival + 0.5 >= ExtractionDeadline - Now)
                    {
                        ++DeadlineRejected;
                        continue;
                    }

                    // 플레이어가 차단 지점을 먼저 지나갈 것으로 예상되는 경우
                    if (PlayerSpeed >= 100.0 &&
                        Arrival > Progress / PlayerSpeed + 0.5)
                    {
                        ++PlayerArrivalRejected;
                        continue;
                    }

                    FOption& Option = Options.AddDefaulted_GetRef();

                    Option.Monster = Monster;
                    Option.Route = Route;
                    Option.Block = Block;
                    Option.ApproachSide = GetApproachSide(Path, ApproachForward);
                    Option.Path = MoveTemp(Path);
                    Option.Progress = Progress;
                    Option.Arrival = Arrival;
                    Option.Score = Arrival - Progress / 2000.0;
                }
            }
        }
    }

    Options.Sort([](const FOption& A, const FOption& B)
    {
        return A.Score < B.Score;
    });

    // 다른 포위 담당과 같은 개체나 같은 차단 위치를 쓰지 않음
    auto CanUseOption = [&](const FOption& Candidate, const TArray<int32>& Chosen)
    {
        const double Radius = Candidate.Monster->GetCapsuleComponent()->GetScaledCapsuleRadius();
        for (const FDynamicBlockerAssignment& Existing : DynamicAssignments)
        {
            ABaruMonsterCharacter* Other = Existing.Monster.Get();
            if (!IsValid(Other)) continue;
            const double Spacing = FMath::Max(280.0,
                Radius + Other->GetCapsuleComponent()->GetScaledCapsuleRadius() + 100.0);
            if (Other == Candidate.Monster ||
                FVector::DistSquared(Existing.Block, Candidate.Block) < FMath::Square(Spacing))
                return false;
        }
        for (const int32 Index : Chosen)
        {
            const FOption& Other = Options[Index];
            const double Spacing = FMath::Max(280.0,
                Radius + Other.Monster->GetCapsuleComponent()->GetScaledCapsuleRadius() + 100.0);
            if (Other.Monster == Candidate.Monster ||
                FVector::DistSquared(Other.Block, Candidate.Block) < FMath::Square(Spacing))
                return false;
        }
        return true;
    };

    const int32 Limit = PreviousBlockers + RequestedBlockers;
    const int32 RemainingSlots = Limit - DynamicAssignments.Num();
    TArray<int32> SelectedOptions;
    int32 LeftCount = 0;
    int32 RightCount = 0;
    bool bOppositePair = false;

    for (const FDynamicBlockerAssignment& Existing : DynamicAssignments)
    {
        LeftCount += Existing.ApproachSide < 0 ? 1 : 0;
        RightCount += Existing.ApproachSide > 0 ? 1 : 0;
    }

    // 새 작전은 가능한 경우 양쪽 한 마리씩 먼저 선택
    if (DynamicAssignments.IsEmpty() && RemainingSlots >= 2)
    {
        double BestPairScore = TNumericLimits<double>::Max();
        int32 BestA = INDEX_NONE;
        int32 BestB = INDEX_NONE;
        for (int32 A = 0; A < Options.Num(); ++A)
        {
            if (!CanUseOption(Options[A], SelectedOptions)) continue;
            TArray<int32> First;
            First.Add(A);
            for (int32 B = A + 1; B < Options.Num(); ++B)
            {
                const double Score = Options[A].Score + Options[B].Score;
                if (Score >= BestPairScore ||
                    Options[A].ApproachSide * Options[B].ApproachSide != -1 ||
                    !CanUseOption(Options[B], First)) continue;
                FVector Unused;
                if (!FindSeparateApproach(Options[A].Path, Options[B].Path, Unused) ||
                    !FindSeparateApproach(Options[B].Path, Options[A].Path, Unused)) continue;
                BestPairScore = Score;
                BestA = A;
                BestB = B;
            }
        }
        if (BestA != INDEX_NONE)
        {
            SelectedOptions.Add(BestA);
            SelectedOptions.Add(BestB);
            ++LeftCount;
            ++RightCount;
            bOppositePair = true;
        }
    }

    // 이번 웨이브의 배정 수 안에서 양쪽 접근을 우선 선택
    while (SelectedOptions.Num() < RemainingSlots)
    {
        int32 Best = INDEX_NONE;
        int32 BestPenalty = 3;
        double BestScore = TNumericLimits<double>::Max();
        for (int32 Index = 0; Index < Options.Num(); ++Index)
        {
            const FOption& Option = Options[Index];
            if (!CanUseOption(Option, SelectedOptions)) continue;
            const int32 Penalty = Option.ApproachSide == 0 ? 2 :
                (Option.ApproachSide < 0 ? (LeftCount > RightCount ? 1 : 0)
                                        : (RightCount > LeftCount ? 1 : 0));
            if (Penalty < BestPenalty ||
                (Penalty == BestPenalty && Option.Score < BestScore))
            {
                Best = Index;
                BestPenalty = Penalty;
                BestScore = Option.Score;
            }
        }
        if (Best == INDEX_NONE) break;
        SelectedOptions.Add(Best);
        LeftCount += Options[Best].ApproachSide < 0 ? 1 : 0;
        RightCount += Options[Best].ApproachSide > 0 ? 1 : 0;
    }

    BARU_NET_LOG(this, LogBaruAI, Log,
        TEXT("New wave selection: Captured=%d, Requested=%d, Existing=%d, Selected=%d, OppositePair=%s"),
        NewWaveMonsters.Num(), RequestedBlockers, PreviousBlockers,
        SelectedOptions.Num(), bOppositePair ? TEXT("true") : TEXT("false"));

    DynamicAssignmentState = CurrentDirectorState;

    for (const int32 OptionIndex : SelectedOptions)
    {
        const FOption& Option = Options[OptionIndex];
        if (DynamicAssignments.Num() >= Limit)
        {
            break;
        }

        bool bConflicts = false;

        for (
            const FDynamicBlockerAssignment& Existing :
            DynamicAssignments
        )
        {
            const ABaruMonsterCharacter* Other = Existing.Monster.Get();

            const double Spacing = IsValid(Other)
                ? FMath::Max(
                    280.0,
                    static_cast<double>(
                        Other->GetCapsuleComponent()->GetScaledCapsuleRadius() +
                        Option.Monster->GetCapsuleComponent()->GetScaledCapsuleRadius() +
                        100.0f
                    )
                )
                : 280.0;

            if (Other == Option.Monster ||
                FVector::DistSquared(Existing.Block, Option.Block) <
                    FMath::Square(Spacing))
            {
                bConflicts = true;
                break;
            }
        }

        if (bConflicts)
        {
            continue;
        }

        ABaruMonsterAIController* Controller =
          FindCommandController(Option.Monster);

        if (!IsValid(Controller))
        {
            continue;
        }

        const bool bCanAssign =
            Controller->GetDirectorCommand() ==
                EBaruMonsterDirectorCommand::None ||
            IsWaveInvestigating(Option.Monster, Controller) ||
            (bExtraction &&
             Controller->GetDirectorCommand() ==
                EBaruMonsterDirectorCommand::ExtractionRally &&
             Controller->GetExtractionRallyTarget() ==
                TargetPlayer);

        if (!bCanAssign ||
            !Controller->ReceiveDirectorDynamicEncirclementCommand(
                TargetPlayer,
                Option.Route,
                Option.Block
            ))
        {
            continue;
        }

        FDynamicBlockerAssignment& Assignment =
            DynamicAssignments.AddDefaulted_GetRef();

        Assignment.Monster = Option.Monster;
        Assignment.Controller = Controller;
        Assignment.Target = TargetPlayer;
        Assignment.Revision = Controller->GetDirectorCommandRevision();
        Assignment.Block = Option.Block;
        Assignment.ApproachSide = Option.ApproachSide;
        Assignment.PlayerPath = PlayerPath;
        Assignment.BlockProgress = Option.Progress;
        Assignment.StartedAt = Now;

        bHasDynamicEncirclement = true;

        BARU_NET_LOG(
            this, LogBaruAI, Log,
            TEXT("Group blocker assigned: Mode=%s, Monster=%s, Target=%s, ETA=%.1f, Block=%s"),
            bExtraction ? TEXT("Extraction") : TEXT("Pressure"),
            *GetNameSafe(Option.Monster),
            *GetNameSafe(TargetPlayer),
            Option.Arrival,
            *Option.Block.ToCompactString()
        );

        if (bDrawDynamicEncirclementDebug)
        {
            for (int32 Index = 1; Index < Option.Path.Num(); ++Index)
            {
                DrawDebugLine(
                    World,
                    Option.Path[Index - 1] + FVector(0, 0, 40),
                    Option.Path[Index] + FVector(0, 0, 40),
                    FColor::Yellow,
                    false,
                    12.0f,
                    0,
                    5.0f
                );
            }

            DrawDebugSphere(
                World, Option.Route, 65.0f, 12,
                FColor::Cyan, false, 12.0f
            );

            DrawDebugSphere(
                World, Option.Block, 90.0f, 12,
                FColor::Red, false, 12.0f
            );
        }
    }

    BARU_NET_LOG(
        this, LogBaruAI, Log,
        TEXT("Group plan: Mode=%s, Candidates=%d, Options=%d, Blockers=%d, Queries=%d"),
        bExtraction ? TEXT("Extraction") : TEXT("Pressure"),
        Candidates.Num(),
        Options.Num(),
        DynamicAssignments.Num(),
        Queries
    );
    
    BARU_NET_LOG(
        this,
        LogBaruAI,
        Log,
        TEXT(
            "Group filter: Separated=%d, Clear=%d, "
            "LifetimeReject=%d, DeadlineReject=%d, PlayerArrivalReject=%d"
        ),
        SeparatedRoutes,
        ClearRoutes,
        LifetimeRejected,
        DeadlineRejected,
        PlayerArrivalRejected
    );

    const int32 Added = DynamicAssignments.Num() - PreviousBlockers;
    BARU_NET_LOG(this, LogBaruAI, Log,
        TEXT("New wave assigned: Requested=%d, Assigned=%d, ActiveTotal=%d"),
        RequestedBlockers, Added, DynamicAssignments.Num());
    return Added > 0;
}


void ABaruMonsterDirector::FinishDynamicMember(
    int32 Index,
    const TCHAR* Reason
)
{
    if (!DynamicAssignments.IsValidIndex(Index))
    {
        return;
    }

    // 명령 해제로 다른 콜백이 실행되기 전에 배정 기록부터 제거
    const FDynamicBlockerAssignment Assignment =
        DynamicAssignments[Index];

    DynamicAssignments.RemoveAt(Index);
    bHasDynamicEncirclement = !DynamicAssignments.IsEmpty();

    ABaruMonsterAIController* Controller =
        Assignment.Controller.Get();

    if (IsValid(Controller) &&
        Controller->HasAuthority() &&
        Controller->GetPawn() == Assignment.Monster.Get() &&
        Controller->IsUsingDynamicEncirclement() &&
        Controller->GetEncirclementTarget() == Assignment.Target.Get() &&
        ((Controller->GetDirectorCommand() ==
            EBaruMonsterDirectorCommand::Encircle &&
          Controller->GetDirectorCommandRevision() ==
            Assignment.Revision) ||
         (Controller->GetDirectorCommand() ==
            EBaruMonsterDirectorCommand::Hold &&
          Controller->GetDirectorCommandRevision() ==
            Assignment.Revision + 1u)))
    {
        Controller->ClearDirectorCommand();
    }

    if (!bHasDynamicEncirclement && IsValid(GetWorld()))
    {
        LastEncirclementDecisionTime = GetWorld()->GetTimeSeconds();
    }

    BARU_NET_LOG(
        this, LogBaruAI, Log,
        TEXT("Group blocker ended: Monster=%s, Reason=%s"),
        *GetNameSafe(Assignment.Monster.Get()),
        Reason
    );
}


void ABaruMonsterDirector::FinishDynamicEncirclement(
    const TCHAR* Reason
)
{
    while (!DynamicAssignments.IsEmpty())
    {
        FinishDynamicMember(
            DynamicAssignments.Num() - 1,
            Reason
        );
    }

    bHasDynamicEncirclement = false;
}


void ABaruMonsterDirector::RefreshDynamicEncirclementAssignment()
{
    using namespace BaruDynamicEncirclement;

    if (GetNetMode() == NM_Client ||
        !IsValid(GetWorld()) ||
        bUpdatingDynamicGroup)
    {
        return;
    }

    // 이 함수가 실행되는 동안 중복 갱신을 막음
    // 함수가 끝나면 기존 값으로 자동 복원
    TGuardValue<bool> UpdateGuard(bUpdatingDynamicGroup, true);

    const bool bExtraction =
        DynamicAssignmentState == EBaruDirectorState::Extraction;

    if (!bUseDynamicEncirclement ||
        !IsEncirclementAllowed() ||
        CurrentDirectorState != DynamicAssignmentState ||
        (bExtraction && !HasValidExtractionContext()))
    {
        FinishDynamicEncirclement(TEXT("State or extraction changed"));
        return;
    }

    const double Now = GetWorld()->GetTimeSeconds();

    for (int32 Index = DynamicAssignments.Num() - 1; Index >= 0; --Index)
    {
        FDynamicBlockerAssignment& Assignment =
            DynamicAssignments[Index];

        ABaruMonsterCharacter* Monster = Assignment.Monster.Get();
        ABaruMonsterAIController* Controller = Assignment.Controller.Get();
        APawn* Target = Assignment.Target.Get();

        const TCHAR* Reason = nullptr;

        if (!IsValid(Monster) ||
            ICombatInterface::Execute_IsDead(Monster) ||
            !RegisteredMonsters.Contains(Assignment.Monster) ||
            !IsValid(Controller) ||
            Controller->GetPawn() != Monster ||
            !IsLivingPlayer(Target) ||
            (bExtraction && ExtractionTargetPlayer.Get() != Target))
        {
            Reason = TEXT("Participant changed");
        }
        else if (!Controller->IsUsingDynamicEncirclement() ||
                 Controller->GetEncirclementTarget() != Target)
        {
            Reason = TEXT("Command replaced");
        }
        else if (Controller->GetDirectorCommand() ==
                    EBaruMonsterDirectorCommand::Hold &&
                 Controller->GetDirectorCommandRevision() ==
                    Assignment.Revision + 1u)
        {
            Reason = TEXT("BT completed");
        }
        else if (Controller->GetDirectorCommand() !=
                    EBaruMonsterDirectorCommand::Encircle ||
                 Controller->GetDirectorCommandRevision() !=
                    Assignment.Revision)
        {
            Reason = TEXT("Command revision changed");
        }
        else if (Now - Assignment.StartedAt >=
                    FMath::Max(6.0f, DynamicPlanLifetime))
        {
            Reason = TEXT("Plan expired");
        }

        if (Reason)
        {
            FinishDynamicMember(Index, Reason);
            continue;
        }

        const FVector TargetLocation = Target->GetNavAgentLocation();

        if (!Assignment.bReachedBlock &&
            FVector::Distance(
                Monster->GetNavAgentLocation(),
                Assignment.Block
            ) <= 180.0)
        {
            Assignment.bReachedBlock = true;

            BARU_NET_LOG(
                this, LogBaruAI, Log,
                TEXT("Group block area reached: Monster=%s"),
                *GetNameSafe(Monster)
            );
        }

        // 차단 대상과 근접 전투가 가능해지면 기존 추적·공격으로 복귀
        if (Controller->GetCurrentTarget() == Target &&
            FVector::Distance(
                Monster->GetActorLocation(),
                Target->GetActorLocation()
            ) < 300.0 &&
            Controller->LineOfSightTo(Target))
        {
            FinishDynamicMember(Index, TEXT("Target entered combat range"));
            continue;
        }

        double DistanceSquared = 0.0;

        const double Progress = ClosestPathProgress(
            Assignment.PlayerPath,
            TargetLocation,
            DistanceSquared
        );

        if (DistanceSquared < FMath::Square(350.0) &&
            Progress > Assignment.BlockProgress + 200.0)
        {
            FinishDynamicMember(Index, TEXT("Target passed block"));
            continue;
        }

        // 계획한 경로를 따라 코너를 도는 동안에는 명령 유지
        if (DistanceSquared <= FMath::Square(600.0))
        {
            Assignment.OffRouteSince = -1.0;
            continue;
        }

        if (Assignment.OffRouteSince < 0.0)
        {
            Assignment.OffRouteSince = Now;
        }

        if (Now - Assignment.OffRouteSince < 2.0)
        {
            continue;
        }

        // 탈출 경로가 바뀌어도 기존 차단 지점을 통과한다면 유지
        if (bExtraction)
        {
            UNavigationPath* NewPath =
                UNavigationSystemV1::FindPathToLocationSynchronously(
                    GetWorld(),
                    TargetLocation,
                    Assignment.PlayerPath.Last(),
                    Target,
                    nullptr
                );

            if (IsCompletePath(NewPath))
            {
                double BlockDistanceSquared = 0.0;

                const double NewProgress = ClosestPathProgress(
                    NewPath->PathPoints,
                    Assignment.Block,
                    BlockDistanceSquared
                );

                if (BlockDistanceSquared < FMath::Square(350.0) &&
                    NewProgress > 200.0)
                {
                    Assignment.PlayerPath = NewPath->PathPoints;
                    Assignment.BlockProgress = NewProgress;
                    Assignment.OffRouteSince = -1.0;
                    continue;
                }
            }
        }

        FinishDynamicMember(
            Index,
            TEXT("Target left route for 2 seconds")
        );
    }
}

void ABaruMonsterDirector::RefreshExtractionRally()
{
    using namespace BaruDynamicEncirclement;

    if (GetNetMode() == NM_Client ||
        !IsValid(GetWorld()) ||
        bRefreshingExtractionRally)
    {
        return;
    }

    TGuardValue<bool> Guard(
        bRefreshingExtractionRally,
        true
    );

    const bool bActive =
        CurrentDirectorState == EBaruDirectorState::Extraction &&
        HasValidExtractionContext();

    APawn* Target =
        bActive ? ExtractionTargetPlayer.Get() : nullptr;

    RemoveInvalidMonsters();

    // 3m 안에서 전투로 전환하고, 6m를 벗어나야 다시 집결
    auto IsInCombat = [Target](
        ABaruMonsterCharacter* Monster,
        ABaruMonsterAIController* Controller
    )
    {
        if (!IsValid(Target) ||
            Controller->GetCurrentTarget() != Target)
        {
            return false;
        }

        const double Radius =
            Controller->GetDirectorCommand() ==
                EBaruMonsterDirectorCommand::ExtractionRally
                    ? 300.0
                    : 600.0;

        return FVector::DistSquared(
            Monster->GetActorLocation(),
            Target->GetActorLocation()
        ) <= FMath::Square(Radius) &&
            Controller->LineOfSightTo(Target);
    };

    int32 Cleared = 0;

    // 종료됐거나 다른 명령으로 교체된 기록 정리
    for (int32 Index = ExtractionRallyOrders.Num() - 1;
         Index >= 0;
         --Index)
    {
        const FExtractionRallyOrder Order =
            ExtractionRallyOrders[Index];

        ABaruMonsterCharacter* Monster =
            Order.Monster.Get();

        ABaruMonsterAIController* Controller =
            Order.Controller.Get();

        const bool bOwnsOrder =
            IsValid(Monster) &&
            IsValid(Controller) &&
            Controller->HasAuthority() &&
            Controller->GetPawn() == Monster &&
            Controller->GetDirectorCommand() ==
                EBaruMonsterDirectorCommand::ExtractionRally &&
            Controller->GetDirectorCommandRevision() ==
                Order.Revision;

        const bool bCanContinue =
            bOwnsOrder &&
            bActive &&
            RegisteredMonsters.Contains(Order.Monster) &&
            !ICombatInterface::Execute_IsDead(Monster) &&
            Controller->GetExtractionRallyTarget() == Target &&
            !IsInCombat(Monster, Controller);

        if (bCanContinue)
        {
            continue;
        }

        ExtractionRallyOrders.RemoveAt(Index);

        // 우회 등 새 명령으로 바뀌었다면 그 명령은 유지
        if (bOwnsOrder)
        {
            Controller->ClearDirectorCommand();
            ++Cleared;
        }
    }

    if (!bActive)
    {
        if (Cleared > 0)
        {
            BARU_NET_LOG(
                this,
                LogBaruAI,
                Log,
                TEXT("Extraction rally ended: Cleared=%d"),
                Cleared
            );
        }

        return;
    }

    int32 Added = 0;
    int32 Blockers = 0;
    int32 Fighting = 0;

    // 명령 전달 중 등록 목록이 바뀌어도 안전하게 순회
    const TArray<TWeakObjectPtr<ABaruMonsterCharacter>> Monsters =
        RegisteredMonsters;

    for (const TWeakObjectPtr<ABaruMonsterCharacter>& Entry :
         Monsters)
    {
        ABaruMonsterCharacter* Monster = Entry.Get();

        if (!IsValid(Monster) ||
            ICombatInterface::Execute_IsDead(Monster))
        {
            continue;
        }

        ABaruMonsterAIController* Controller =
            Cast<ABaruMonsterAIController>(
                Monster->GetController()
            );

        if (!IsValid(Controller) ||
            !Controller->HasAuthority())
        {
            continue;
        }

        const EBaruMonsterDirectorCommand Command =
            Controller->GetDirectorCommand();

        // 우회 담당은 기존 경로를 계속 수행
        if (Command == EBaruMonsterDirectorCommand::Encircle)
        {
            ++Blockers;
            continue;
        }

        // 목표와 근접 전투 중인 몬스터는 전투 유지
        if (IsInCombat(Monster, Controller))
        {
            ++Fighting;
            continue;
        }

        if (Command ==
            EBaruMonsterDirectorCommand::ExtractionRally)
        {
            continue;
        }

        if (!Controller->
            ReceiveDirectorExtractionRallyCommand(Target))
        {
            continue;
        }

        FExtractionRallyOrder& Order =
            ExtractionRallyOrders.AddDefaulted_GetRef();

        Order.Monster = Monster;
        Order.Controller = Controller;
        Order.Revision =
            Controller->GetDirectorCommandRevision();

        ++Added;
    }

    if (Added > 0 || Cleared > 0)
    {
        BARU_NET_LOG(
            this,
            LogBaruAI,
            Log,
            TEXT(
                "Extraction rally: New=%d, Active=%d, "
                "Blockers=%d, Fighting=%d"
            ),
            Added,
            ExtractionRallyOrders.Num(),
            Blockers,
            Fighting
        );
    }
}