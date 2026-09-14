// BaruInventoryItemWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/BaruCommonUserWidget.h"
#include "BaruInventoryItemWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;
class UBaruItemInstance;
class UButton;
class UBaruInventoryComponent;
class UBaruInventoryTooltipWidget;	// 아이템 툴팁

UCLASS(Abstract)
class BARUGAME_API UBaruInventoryItemWidget
	: public UBaruCommonUserWidget
{
	GENERATED_BODY()

public:
	// InventoryWidget이 아이템 하나를 화면에 표시할 때 호출합니다.
	void InitializeItem(
	UBaruInventoryComponent* InInventoryComponent,
	UBaruItemInstance* InItemInstance,
	UTexture2D* InThumbnail,
	const FText& InItemName,
	int32 InQuantity);

	// 이후 장착·버리기·상세 정보 표시에서 사용할 실제 아이템 객체입니다.
	UFUNCTION(BlueprintPure, Category = "BARU|Inventory UI")
	UBaruItemInstance* GetItemInstance() const
	{
		return ItemInstance;
	}

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Item;	
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Thumbnail;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Quantity;

	virtual void NativeOnDragDetected(	const FGeometry& InGeometry,const FPointerEvent& InMouseEvent,	UDragDropOperation*& OutOperation) override;
	
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry,const FPointerEvent& InMouseEvent) override;

	virtual FReply NativeOnMouseButtonUp(	const FGeometry& InGeometry,const FPointerEvent& InMouseEvent) override;
	
		//아이템 툴팁.
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry,const FPointerEvent& InMouseEvent) override;

	virtual void NativeOnMouseLeave(	const FPointerEvent& InMouseEvent) override;

private:
	UPROPERTY()
	TObjectPtr<UBaruItemInstance> ItemInstance;
	
	UFUNCTION()
	void HandleItemClicked();

	UPROPERTY()
	TObjectPtr<UBaruInventoryComponent> InventoryComponent;
	
	bool bPendingItemClick = false;
	
		//아이템 툴팁
	void ShowItemTooltip();
	void HideItemTooltip();

	UPROPERTY(Transient)
	TObjectPtr<UBaruInventoryTooltipWidget> ItemTooltip;
};