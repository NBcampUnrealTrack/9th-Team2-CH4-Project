// BaruInventoryDragDropOperation.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Gameplay/Equipment/DataTypes/BaruEquipmentTypes.h"
#include "BaruInventoryDragDropOperation.generated.h"

class UBaruItemInstance;

UCLASS()
class BARUGAME_API UBaruInventoryDragDropOperation
	: public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UBaruItemInstance> ItemInstance;

	// None이면 인벤토리 Grid에서 시작한 드래그입니다.
	UPROPERTY(BlueprintReadOnly)
	EBaruEquipmentSlot SourceEquipmentSlot =
		EBaruEquipmentSlot::None;
};