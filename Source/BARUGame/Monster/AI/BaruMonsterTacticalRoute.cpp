


#include "BaruMonsterTacticalRoute.h"

#include "Components/SplineComponent.h"
#include "Monster/Characters/BaruMonsterCharacter.h"


ABaruMonsterTacticalRoute::ABaruMonsterTacticalRoute()
{
    PrimaryActorTick.bCanEverTick = false;

    bReplicates = false;
    SetActorEnableCollision(false);

    RouteSpline = CreateDefaultSubobject<USplineComponent>(
        TEXT("RouteSpline")
    );

    SetRootComponent(RouteSpline);

    // 테스트용 기본 경로:
    // 첫 점은 액터 위치,
    // 마지막 점은 액터 전방 500cm
    RouteSpline->ClearSplinePoints(false);

    RouteSpline->AddSplinePoint(
        FVector::ZeroVector,
        ESplineCoordinateSpace::Local,
        false
    );

    RouteSpline->AddSplinePoint(
        FVector(500.0f, 0.0f, 0.0f),
        ESplineCoordinateSpace::Local,
        false
    );

    RouteSpline->SetSplinePointType(
        0,
        ESplinePointType::Linear,
        false
    );

    RouteSpline->SetSplinePointType(
        1,
        ESplinePointType::Linear,
        false
    );

    RouteSpline->SetClosedLoop(false, false);
    RouteSpline->UpdateSpline();
}

FVector ABaruMonsterTacticalRoute::GetRouteEntryLocation() const
{
    if (!IsValid(RouteSpline) ||
        RouteSpline->GetNumberOfSplinePoints() <= 0)
    {
        return GetActorLocation();
    }

    return RouteSpline->GetLocationAtSplinePoint(
        0,
        ESplineCoordinateSpace::World
    );
}

FVector ABaruMonsterTacticalRoute::GetBlockLocation() const
{
    if (!IsValid(RouteSpline) ||
        RouteSpline->GetNumberOfSplinePoints() <= 0)
    {
        return GetActorLocation();
    }

    const int32 LastPointIndex =
        RouteSpline->GetNumberOfSplinePoints() - 1;

    return RouteSpline->GetLocationAtSplinePoint(
        LastPointIndex,
        ESplineCoordinateSpace::World
    );
}

bool ABaruMonsterTacticalRoute::IsRouteConfigured() const
{
    if (!IsValid(RouteSpline) ||
        RouteSpline->GetNumberOfSplinePoints() < 2)
    {
        return false;
    }

    const FVector EntryLocation =
        GetRouteEntryLocation();

    const FVector BlockLocation =
        GetBlockLocation();

    if (EntryLocation.ContainsNaN() ||
        BlockLocation.ContainsNaN())
    {
        return false;
    }

    // 두 지점이 사실상 같은 위치라면 경로로 사용하지 않음
    return FVector::DistSquared(
        EntryLocation,
        BlockLocation
    ) > FMath::Square(100.0f);
}

bool ABaruMonsterTacticalRoute::IsAvailableFor(
    const ABaruMonsterCharacter* Monster
) const
{
    if (!bRouteEnabled ||
        !IsRouteConfigured() ||
        !IsValid(Monster))
    {
        return false;
    }

    return !ReservedBy.IsValid() ||
           ReservedBy.Get() == Monster;
}

bool ABaruMonsterTacticalRoute::TryReserve(
    ABaruMonsterCharacter* Monster
)
{
    if (!IsAvailableFor(Monster))
    {
        return false;
    }

    ReservedBy = Monster;
    return true;
}

void ABaruMonsterTacticalRoute::ReleaseReservation(
    const ABaruMonsterCharacter* Monster
)
{
    if (!ReservedBy.IsValid() ||
        ReservedBy.Get() == Monster)
    {
        ReservedBy.Reset();
    }
}

