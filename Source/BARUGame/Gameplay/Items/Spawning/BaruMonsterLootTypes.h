#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "BaruMonsterLootTypes.generated.h"

// 몬스터별 DT_MonsterLoot_*의 행 구조. 아이템 상세 정보는 DT_Item을 참조합니다.
USTRUCT(BlueprintType)
struct BARUGAME_API FBaruMonsterLootRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
    FDataTableRowHandle ItemRow;

    // 각 행을 독립적으로 한 번 추첨합니다. 0.25 = 25%, 1 = 확정.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DropChance = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot",
        meta = (ClampMin = "1", ClampMax = "10000"))
    int32 MinQuantity = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot",
        meta = (ClampMin = "1", ClampMax = "10000"))
    int32 MaxQuantity = 1;
};

// 사망 시 확정된 결과. 향후 선택 습득 UI에서도 이 목록을 사용합니다.
USTRUCT(BlueprintType)
struct BARUGAME_API FBaruCorpseLootEntry
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot")
    FGuid EntryID;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot")
    FName ItemID = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot")
    int32 Quantity = 0;
};
