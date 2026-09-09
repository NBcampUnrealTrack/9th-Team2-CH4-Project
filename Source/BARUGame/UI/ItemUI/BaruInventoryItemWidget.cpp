// BaruInventoryItemWidget.cpp

#include "UI/ItemUI/BaruInventoryItemWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Gameplay/Items/BaruItemInstance.h"

#include "Components/Button.h"
#include "Engine/DataTable.h"

#include "Gameplay/Inventory/BaruInventoryComponent.h"
#include "Gameplay/Items/DataTypes/BaruItemData.h"

void UBaruInventoryItemWidget::InitializeItem(
	UBaruInventoryComponent* InInventoryComponent,
	UBaruItemInstance* InItemInstance,
	UTexture2D* InThumbnail,
	const FText& InItemName,
	int32 InQuantity)
{
	InventoryComponent = InInventoryComponent;
	ItemInstance = InItemInstance;

	if (IsValid(Image_Thumbnail))
	{
		if (IsValid(InThumbnail))
		{
			Image_Thumbnail->SetBrushFromTexture(InThumbnail);
			Image_Thumbnail->SetVisibility(
				ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Image_Thumbnail->SetVisibility(
				ESlateVisibility::Hidden);
		}
	}

	if (IsValid(Text_Quantity))
	{
		if (InQuantity > 1)
		{
			Text_Quantity->SetText(
				FText::AsNumber(InQuantity));

			Text_Quantity->SetVisibility(
				ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Text_Quantity->SetVisibility(
				ESlateVisibility::Collapsed);
		}
	}
	
	// 정식 상세 정보창을 만들기 전까지 아이템 이름 확인용입니다.
	SetToolTipText(InItemName);
}

	// 클릭 바인딩 함수.
void UBaruInventoryItemWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IsValid(Button_Item))
	{
		Button_Item->OnClicked.AddUniqueDynamic(
			this,
			&UBaruInventoryItemWidget::HandleItemClicked);
	}
}

void UBaruInventoryItemWidget::NativeDestruct()
{
	if (IsValid(Button_Item))
	{
		Button_Item->OnClicked.RemoveDynamic(
			this,
			&UBaruInventoryItemWidget::HandleItemClicked);
	}

	InventoryComponent = nullptr;
	ItemInstance = nullptr;

	Super::NativeDestruct();
}

//--------클릭 처리 추가.
void UBaruInventoryItemWidget::HandleItemClicked()
{
	if (!IsValid(InventoryComponent)
		|| !IsValid(ItemInstance)
		|| !IsValid(InventoryComponent->ItemDataTable))
	{
		return;
	}

	const FItemData* ItemData =
		InventoryComponent->ItemDataTable
			->FindRow<FItemData>(
				ItemInstance->ItemID,
				TEXT("InventoryItemClicked"),
				false);

	if (!ItemData)
	{
		return;
	}

		// 소비 아이템 효과가 아직 구현되지 않았음.
		// 현재는 Weapon만 클릭 사용을 허용.
	if (ItemData->ItemType != EItemType::Weapon)
	{
		return;
	}

	InventoryComponent->RequestUseItem(ItemInstance);
}