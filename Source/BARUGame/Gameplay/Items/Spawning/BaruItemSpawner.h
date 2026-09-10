// BaruItemSpawner.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "Gameplay/Items/DataTypes/BaruItemData.h"
#include "BaruItemSpawner.generated.h"

class UBoxComponent;

    // 랜덤 생성 목록 DataTable의 한 행입니다.
    // ItemRow는 DT_Item의 실제 아이템 행을 참조.
USTRUCT(BlueprintType)
struct BARUGAME_API FBaruItemSpawnRow : public FTableRowBase
{
    GENERATED_BODY()

public:
    // DT_Item의 DataTable과 Row Name을 에디터에서 선택합니다.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
    FDataTableRowHandle ItemRow;

    // Pickup 하나에 들어갈 최소 수량.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn",
        meta = (ClampMin = "1", ClampMax = "10000"))
    int32 MinQuantity = 1;

    // Pickup 하나에 들어갈 최대 수량.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn",
        meta = (ClampMin = "1", ClampMax = "10000"))
    int32 MaxQuantity = 1;
};

UCLASS()
class BARUGAME_API ABaruItemSpawner : public AActor
{
    GENERATED_BODY()

public:
    ABaruItemSpawner();

    // 서버 내부 또는 서버 BP에서 호출합니다.
    // 클라이언트에서 호출해도 생성하지 않습니다.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly,
        Category = "BARU|Item Spawner")
    void SpawnItemsOnce();
    
        // MonsterItemSpawnerComponent가 FinishSpawningActor 전에 호출.
    void PrepareForMonsterDrop(AActor* Corpse);
    
    int32 GetLastSpawnedCount() const { return LastSpawnedCount; }

protected:
    virtual void BeginPlay() override;

    // 생성 범위를 표시하는 박스.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        Category = "BARU|Item Spawner")
    TObjectPtr<UBoxComponent> SpawnArea;

    // 기존 FItemData 기반 DataTable.
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "BARU|Item Spawner")
    TObjectPtr<UDataTable> ItemDataTable;

    // FBaruItemSpawnRow 기반 DataTable.
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "BARU|Item Spawner")
    TObjectPtr<UDataTable> SpawnTable;

    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "BARU|Item Spawner")
    bool bSpawnOnBeginPlay = true;
    
        //초기 픽업 액터의 생성 개수.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Item Spawner",
    meta = (ClampMin = "0", ClampMax = "100"))
    int32 InitialSpawnCount = 5;
    
    //무기 드랍 여부.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Item Spawner|Filters")
    bool bAllowWeaponDrops = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Item Spawner|Surface")
    bool bRequireSpawnSurfaceTag = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Item Spawner|Surface")
    FName SpawnSurfaceTag = TEXT("Baru.ItemSpawnSurface");


    // Pickup 하나의 위치를 찾기 위한 최대 시도 횟수.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Item Spawner",
        meta = (ClampMin = "1", ClampMax = "100"))
    int32 PlacementAttempts = 20;

    // 이 스포너가 만든 Pickup끼리의 최소 거리.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Item Spawner",
        meta = (ClampMin = "0.0"))
    float MinimumSpacing = 100.0f;

    // 바닥에서 Actor 원점을 얼마나 올릴지 설정.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Item Spawner",
        meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float SpawnHeight = 5.0f;

    // 벽 등에 너무 가까운 위치를 피하기 위한 검사 반경.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Item Spawner",
        meta = (ClampMin = "1.0", ClampMax = "200.0"))
    float ClearanceRadius = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Item Spawner|Rarity",
        meta = (ClampMin = "0.0"))
    float CommonWeight = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Item Spawner|Rarity",
        meta = (ClampMin = "0.0"))
    float UncommonWeight = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Item Spawner|Rarity",
        meta = (ClampMin = "0.0"))
    float RareWeight = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Item Spawner|Rarity",
        meta = (ClampMin = "0.0"))
    float EpicWeight = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Item Spawner|Rarity",
        meta = (ClampMin = "0.0"))
    float LegendaryWeight = 1.0f;

private:
    // 같은 스포너의 중복 실행 방지.
    UPROPERTY(Transient)
    bool bHasSpawned = false;

    UPROPERTY(Transient)
    bool bMonsterDropMode = false;

    UPROPERTY(Transient)
    TWeakObjectPtr<AActor> IgnoredSurfaceActor;

    UPROPERTY(Transient)
    int32 LastSpawnedCount = 0;

    float GetRarityWeight(EBaruItemRarity Rarity) const;
    bool FindSpawnLocation(const TArray<FVector>& OccupiedPositions,
        FVector& OutLocation) const;

};