#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/BaruActivatableWidget.h"
#include "BaruInventoryScreenWidget.generated.h"


class USizeBox;
class UDragDropOperation;

UCLASS(Abstract)
class BARUGAME_API UBaruInventoryScreenWidget
	: public UBaruActivatableWidget
{
	GENERATED_BODY()

public:
	UBaruInventoryScreenWidget(
		const FObjectInitializer& ObjectInitializer);
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> SizeBox_Content;

	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;
};