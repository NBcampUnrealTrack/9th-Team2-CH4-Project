#pragma once

#include "CoreMinimal.h"
#include "BaruInventoryNotification.generated.h"

// InventoryComponent가 확정한 월드 아이템 수납 결과입니다.
UENUM(BlueprintType)
enum class EBaruInventoryPickupResult : uint8
{
	Succeeded UMETA(DisplayName = "Succeeded"),
	Partial   UMETA(DisplayName = "Partial"),
	Full      UMETA(DisplayName = "Inventory Full")
};

USTRUCT(BlueprintType)
struct BARUGAME_API FBaruInventoryPickupNotification
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BARU|Inventory|Pickup")
	EBaruInventoryPickupResult Result =
		EBaruInventoryPickupResult::Succeeded;

	UPROPERTY(BlueprintReadOnly, Category = "BARU|Inventory|Pickup")
	FName ItemID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "BARU|Inventory|Pickup")
	FText ItemName;

	UPROPERTY(BlueprintReadOnly, Category = "BARU|Inventory|Pickup")
	int32 RequestedQuantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BARU|Inventory|Pickup")
	int32 AddedQuantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BARU|Inventory|Pickup")
	int32 RemainingQuantity = 0;
};
