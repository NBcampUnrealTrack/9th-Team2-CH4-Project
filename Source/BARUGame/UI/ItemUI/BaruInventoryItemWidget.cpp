// BaruInventoryItemWidget.cpp

#include "UI/ItemUI/BaruInventoryItemWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Gameplay/Items/BaruItemInstance.h"

#include "Components/Button.h"
#include "Engine/DataTable.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "InputCoreTypes.h"
#include "Input/Reply.h"
#include "UI/InventoryUI/BaruInventoryDragDropOperation.h"
#include "Components/Image.h"

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

	// 클릭 바인딩을 제거하는 변경.
void UBaruInventoryItemWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 이 아이템에 배정된 영역 밖으로 자식 이미지가 그려지지 않도록 제한.
	SetClipping(EWidgetClipping::ClipToBoundsAlways);

	bPendingItemClick = false;
	SetVisibility(ESlateVisibility::Visible);

	if (IsValid(Button_Item))
	{
		Button_Item->OnClicked.RemoveDynamic(
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
	bPendingItemClick = false;

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

	// 구현. 인벤토리 아이템 드래그.
FReply UBaruInventoryItemWidget::NativeOnPreviewMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& IsValid(ItemInstance)
		&& IsValid(InventoryComponent))
	{
		bPendingItemClick = true;

		// 자식 Button에 전달하기 전에 클릭/드래그 입력을 처리합니다.
		// 마우스를 놓는 이벤트도 이 위젯이 받도록 캡처합니다.
		return FReply::Handled()
			.CaptureMouse(TakeWidget())
			.DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	return Super::NativeOnPreviewMouseButtonDown(
		InGeometry,
		InMouseEvent);
}

FReply UBaruInventoryItemWidget::NativeOnMouseButtonUp(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& bPendingItemClick)
	{
		bPendingItemClick = false;

		const bool bReleasedInside =
			InGeometry.IsUnderLocation(
				InMouseEvent.GetScreenSpacePosition());

		FReply Reply = FReply::Handled().ReleaseMouseCapture();

		// 드래그하지 않고 같은 아이템 위에서 놓았을 때만 장착합니다.
		if (bReleasedInside)
		{
			HandleItemClicked();
		}

		return Reply;
	}

	return Super::NativeOnMouseButtonUp(
		InGeometry,
		InMouseEvent);
}

void UBaruInventoryItemWidget::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(
		InGeometry,
		InMouseEvent,
		OutOperation);

	// 드래그가 시작됐으므로 버튼을 놓아도 클릭 장착하지 않습니다.
	bPendingItemClick = false;

	if (!IsValid(ItemInstance)
		|| !IsValid(InventoryComponent))
	{
		return;
	}

	UBaruInventoryDragDropOperation* DragOperation =
		NewObject<UBaruInventoryDragDropOperation>(this);

	DragOperation->ItemInstance = ItemInstance;
	DragOperation->SourceEquipmentSlot =
		EBaruEquipmentSlot::None;
	// 커서 위치를 드래그 이미지의 좌상단으로 사용.
	// Grid 반환 코드도 커서 위치를 아이템의 좌상단 칸으로 사용합니다.
	DragOperation->Pivot = EDragPivot::TopLeft;
	DragOperation->Offset = FVector2D::ZeroVector;

	// 기존 코드는 드래그 이미지가 없어 이동 중 아이콘이 안 보였습니다.
	if (IsValid(Image_Thumbnail))
	{
		UImage* DragVisual = NewObject<UImage>(DragOperation);

		FSlateBrush DragBrush = Image_Thumbnail->GetBrush();
		DragBrush.SetImageSize(FVector2D(96.0f, 96.0f));
		DragBrush.DrawAs = ESlateBrushDrawType::Image;

		DragVisual->SetBrush(DragBrush);
		DragVisual->SetVisibility(
			ESlateVisibility::HitTestInvisible);

		DragOperation->DefaultDragVisual = DragVisual;
	}

	OutOperation = DragOperation;
}