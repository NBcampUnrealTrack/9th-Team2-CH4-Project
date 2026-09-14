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
	
protected:
	// [09.14] UI에 포커스가 잡혀있어도 I 키나 ESC 키를 눌렀을 때 닫히도록 처리
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
};