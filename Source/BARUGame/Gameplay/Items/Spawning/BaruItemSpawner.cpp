// BaruItemSpawner.cpp


#include "Gameplay/Items/Spawning/BaruItemSpawner.h"

#include "BaruLog.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "Gameplay/Items/BaruBaseItem.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    struct FBaruSpawnCandidate
    {
        FName ItemID;
        TSubclassOf<ABaruBaseItem> PickupClass;
        int32 MinQuantity = 1;
        int32 MaxQuantity = 1;
        double Weight = 0.0;
    };
}



ABaruItemSpawner::ABaruItemSpawner()
{
    PrimaryActorTick.bCanEverTick = false;


    // 서버/클라이언트의 권한을 구분합니다.
    // 실제 아이템 생성은 서버에서만 수행합니다.
    bReplicates = true;

    SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnArea"));

    SetRootComponent(SpawnArea);

    SpawnArea->SetBoxExtent(FVector(400.0f, 400.0f, 150.0f));
    SpawnArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SpawnArea->SetGenerateOverlapEvents(false);
    SpawnArea->SetHiddenInGame(true);
}

void ABaruItemSpawner::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority() && bSpawnOnBeginPlay && !bMonsterDropMode)
    {
        SpawnItemsOnce();
    }
}


void ABaruItemSpawner::PrepareForMonsterDrop(AActor* Corpse)
{
    if (!HasAuthority() || bHasSpawned || !IsValid(Corpse))
    {
        return;
    }
    bMonsterDropMode = true;
    bSpawnOnBeginPlay = false;
    bAllowWeaponDrops = false;
    IgnoredSurfaceActor = Corpse;
    // 임시 생성용 Actor. 실제 Pickup이 네트워크 복제를 담당.
    SetReplicates(false);
}

float ABaruItemSpawner::GetRarityWeight(EBaruItemRarity Rarity) const
{
    switch (Rarity)
    {
    case EBaruItemRarity::Common: return CommonWeight;
    case EBaruItemRarity::Uncommon: return UncommonWeight;
    case EBaruItemRarity::Rare: return RareWeight;
    case EBaruItemRarity::Epic: return EpicWeight;
    case EBaruItemRarity::Legendary: return LegendaryWeight;
    default: return 0.0f;
    }
}



void ABaruItemSpawner::SpawnItemsOnce()
{
    if (!HasAuthority() || bHasSpawned || !GetWorld())
    {
        return;
    }
    if (!IsValid(ItemDataTable) || ItemDataTable->GetRowStruct() != FItemData::StaticStruct()
        || !IsValid(SpawnTable) || SpawnTable->GetRowStruct() != FBaruItemSpawnRow::StaticStruct())
    {
        BARU_NET_LOG(this, LogBaruItem, Warning,
            TEXT("ItemSpawner 실패: ItemDataTable=DT_Item, SpawnTable의 구조체/에셋을 확인해주세요."));
        return;
    }
    if (bRequireSpawnSurfaceTag && SpawnSurfaceTag.IsNone())
    {
        BARU_NET_LOG(this, LogBaruItem, Warning, TEXT("ItemSpawner 실패: SpawnSurfaceTag가 비어 있습니다."));
        return;
    }

    const int32 TargetCount = FMath::Clamp(InitialSpawnCount, 0, 100);
    if (TargetCount == 0)
    {
        bHasSpawned = true;
        return;
    }

    TArray<FBaruItemSpawnRow*> Rows;
    SpawnTable->GetAllRows<FBaruItemSpawnRow>(TEXT("ItemSpawner"), Rows);
    TArray<FBaruSpawnCandidate> Candidates;
    TSet<FName> UsedItemIDs;
    double TotalWeight = 0.0;

    for (const FBaruItemSpawnRow* Row : Rows)
    {
        if (!Row)
        {
            continue;
        }
        if (Row->ItemRow.DataTable != ItemDataTable.Get()
            || Row->ItemRow.RowName.IsNone()
            || Row->MinQuantity < 1 || Row->MaxQuantity < Row->MinQuantity
            || Row->MaxQuantity > 10000)
        {
            BARU_NET_LOG(this, LogBaruItem, Warning,
                TEXT("ItemSpawner 후보 제외: DT_Item 참조/행 이름/수량 오류. Row=%s"),
                *Row->ItemRow.RowName.ToString());
            continue;
        }

        const FItemData* Data = ItemDataTable->FindRow<FItemData>(
            Row->ItemRow.RowName, TEXT("ItemSpawner"), false);
        if (!Data || !Data->ItemActorClass
            || Data->ItemActorClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated))
        {
            BARU_NET_LOG(this, LogBaruItem, Warning,
                TEXT("ItemSpawner 후보 제외: 아이템 행/Pickup 클래스 오류. Row=%s"),
                *Row->ItemRow.RowName.ToString());
            continue;
        }
        if ((!bAllowWeaponDrops || bMonsterDropMode) && Data->ItemType == EItemType::Weapon)
        {
            continue;
        }
        const float Weight = GetRarityWeight(Data->Rarity);
        if (!FMath::IsFinite(Weight) || Weight <= 0.0f)
        {
            continue;
        }
        if (UsedItemIDs.Contains(Row->ItemRow.RowName))
        {
            BARU_NET_LOG(this, LogBaruItem, Warning,
                TEXT("ItemSpawner 중복 후보 제외: Row=%s"), *Row->ItemRow.RowName.ToString());
            continue;
        }

        FBaruSpawnCandidate& Candidate = Candidates.AddDefaulted_GetRef();
        Candidate.ItemID = Row->ItemRow.RowName;
        Candidate.PickupClass = Data->ItemActorClass;
        Candidate.MinQuantity = Row->MinQuantity;
        Candidate.MaxQuantity = Row->MaxQuantity;
        Candidate.Weight = static_cast<double>(Weight);
        TotalWeight += Candidate.Weight;
        UsedItemIDs.Add(Candidate.ItemID);
    }
    if (Candidates.IsEmpty() || TotalWeight <= 0.0)
    {
        BARU_NET_LOG(this, LogBaruItem, Warning,
            TEXT("ItemSpawner 실패: 필터 적용 후 유효한 후보가 없습니다."));
        return;
    }

    // 이후 같은 스포너(생성한 Pickup의 BeginPlay)에서 다시 호출해도 중복 생성하지 않습니다.
    bHasSpawned = true;
    LastSpawnedCount = 0;
    TArray<FVector> OccupiedPositions;

    for (int32 SpawnIndex = 0; SpawnIndex < TargetCount; ++SpawnIndex)
    {
        // 가중치에 따라 아이템 하나 선택.
        double Pick = static_cast<double>(FMath::FRand()) * TotalWeight;
        int32 SelectedIndex = Candidates.Num() - 1;
        for (int32 CandidateIndex = 0; CandidateIndex < Candidates.Num(); ++CandidateIndex)
        {
            Pick -= Candidates[CandidateIndex].Weight;
            if (Pick < 0.0)
            {
                SelectedIndex = CandidateIndex;
                break;
            }
        }

        FVector Location;
        if (!FindSpawnLocation(OccupiedPositions, Location))
        {
            continue;
        }
        const FBaruSpawnCandidate& Selected = Candidates[SelectedIndex];
        const int32 Quantity = FMath::RandRange(Selected.MinQuantity, Selected.MaxQuantity);
        const FTransform Transform(FRotator(0, FMath::FRandRange(0.0f, 360.0f), 0), Location);

       // 생성 완료 전(주워지기 전)에 아이템 ID와 수량을 전달합니다.
        ABaruBaseItem* Pickup = GetWorld()->SpawnActorDeferred<ABaruBaseItem>(
            Selected.PickupClass, Transform, nullptr, nullptr,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (!IsValid(Pickup))
        {
            continue;
        }


	// BP Construction Script에서 기본값을 대입한 경우에도
        // 서버가 선택한 ID와 수량을 최종값으로 유지?
        Pickup->SetReplicates(true);
        Pickup->ItemRow.DataTable = ItemDataTable.Get();
        Pickup->ItemRow.RowName = Selected.ItemID;
        Pickup->PickupCount = Quantity;
        UGameplayStatics::FinishSpawningActor(Pickup, Transform);
        if (!IsValid(Pickup))
        {
            continue;
        }

        Pickup->SetReplicates(true);
        Pickup->ItemRow.DataTable = ItemDataTable.Get();
        Pickup->ItemRow.RowName = Selected.ItemID;
        Pickup->PickupCount = Quantity;
        Pickup->ForceNetUpdate();
        OccupiedPositions.Add(Location);
        ++LastSpawnedCount;
        BARU_NET_LOG(this, LogBaruItem, Log,
            TEXT("ItemSpawner 생성: ItemID=%s Count=%d Actor=%s"),
            *Selected.ItemID.ToString(), Quantity, *Pickup->GetName());
    }

    BARU_NET_LOG(this, LogBaruItem, Log,
        TEXT("ItemSpawner 완료: 요청=%d 생성=%d"), TargetCount, LastSpawnedCount);
}

bool ABaruItemSpawner::FindSpawnLocation(
    const TArray<FVector>& OccupiedPositions, FVector& OutLocation) const
{
    UWorld* World = GetWorld();
    if (!World || !IsValid(SpawnArea)
        || !FMath::IsFinite(MinimumSpacing) || !FMath::IsFinite(SpawnHeight)
        || !FMath::IsFinite(ClearanceRadius))
    {
        return false;
    }
    const FVector Extent = SpawnArea->GetUnscaledBoxExtent();
    const FTransform AreaTransform = SpawnArea->GetComponentTransform();
    const float Spacing = FMath::Max(0.0f, MinimumSpacing);
    const float Radius = FMath::Clamp(ClearanceRadius, 1.0f, 200.0f);
    const int32 Attempts = FMath::Clamp(PlacementAttempts, 1, 100);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(BaruItemSpawner), false, this);
    if (IgnoredSurfaceActor.IsValid())
    {
        Query.AddIgnoredActor(IgnoredSurfaceActor.Get());
    }

    for (int32 Attempt = 0; Attempt < Attempts; ++Attempt)
    {
        const float X = FMath::FRandRange(-Extent.X, Extent.X);
        const float Y = FMath::FRandRange(-Extent.Y, Extent.Y);
        // 박스 위쪽에서 아래쪽으로 바닥을 검사합니다.

        const FVector Start = AreaTransform.TransformPosition(FVector(X, Y, Extent.Z));
        const FVector End = AreaTransform.TransformPosition(FVector(X, Y, -Extent.Z));
        FHitResult Hit;
        if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Query)
            || Hit.bStartPenetrating || Hit.ImpactNormal.Z < 0.7f)
        {
            continue;
        }

        if (bRequireSpawnSurfaceTag)
        {
            const bool bActorTagged = IsValid(Hit.GetActor())
                && Hit.GetActor()->ActorHasTag(SpawnSurfaceTag);
            const bool bComponentTagged = IsValid(Hit.GetComponent())
                && Hit.GetComponent()->ComponentHasTag(SpawnSurfaceTag);
            if (!bActorTagged && !bComponentTagged)
            {
                // 처음 맞은 장애물을 통과해 그 아래에 생성하지 않습니다.
                continue;
            }
        }

        const FVector Candidate = Hit.ImpactPoint
            + FVector(0, 0, FMath::Clamp(SpawnHeight, 0.0f, 100.0f));
        bool bTooClose = false;
        for (const FVector& Existing : OccupiedPositions)
        {
            if (FVector::DistSquared(Candidate, Existing) < FMath::Square(Spacing))
            {
                bTooClose = true;
                break;
            }
        }
        if (bTooClose)
        {
            continue;
        }

        // 바닥 위의 작은 공간에 벽/장애물이 있는지 검사합니다.
        const FVector ClearanceCenter = Hit.ImpactPoint + Hit.ImpactNormal * (Radius + 2.0f);
        if (World->OverlapBlockingTestByChannel(ClearanceCenter, FQuat::Identity,
            ECC_Visibility, FCollisionShape::MakeSphere(Radius), Query))
        {
            continue;
        }
        OutLocation = Candidate;
        return true;
    }
    return false;
}
