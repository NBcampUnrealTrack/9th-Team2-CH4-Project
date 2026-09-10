// BaruInventoryWidget.cpp

#include "UI/InventoryUI/BaruInventoryWidget.h"

#include "Player/BaruPlayerState.h"
#include "Gameplay/Inventory/BaruInventoryComponent.h"
#include "Gameplay/Items/BaruItemInstance.h"
#include "Engine/DataTable.h"
#include "UI/InventoryUI/BaruInventoryCellWidget.h"

#include "Components/SizeBox.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"

#include "UI/ItemUI/BaruInventoryItemWidget.h"

#include "Components/CanvasPanel.h"	// 키입력 구현부.
#include "Components/CanvasPanelSlot.h"





void UBaruInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindInventory();

	// 실제 InventoryComponent의 7×10 설정으로 빈 칸을 생성.
	RebuildEmptyGrid();

	RebuildItemWidgets();

	// 그 위에 실제 아이템을 그리도록 BP에 알림.
	RefreshInventoryView();
}




void UBaruInventoryWidget::NativeDestruct()
{
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->OnInventoryUpdated.RemoveAll(this);
	}

	InventoryComponent = nullptr;

	Super::NativeDestruct();
}




void UBaruInventoryWidget::BindInventory()
{
	if (IsValid(InventoryComponent))
	{
		InventoryComponent->OnInventoryUpdated.RemoveAll(this);
		InventoryComponent = nullptr;
	}

	ABaruPlayerState* BaruPS =
		GetOwningPlayerState<ABaruPlayerState>();

	if (!IsValid(BaruPS))
	{
		return;
	}

	InventoryComponent = BaruPS->GetInventoryComponent();

	if (IsValid(InventoryComponent))
	{
		InventoryComponent->OnInventoryUpdated.AddUObject(
			this,
			&UBaruInventoryWidget::HandleInventoryUpdated);
	}
}

void UBaruInventoryWidget::HandleInventoryUpdated()
{
	RebuildItemWidgets();
	RefreshInventoryView();
}



	// UI가 SlotList나 Cells에 직접 접근하지 않고, 서버가 복제한 데이터만 읽도록 함.
	// 인벤 크기, 템 위치, 점유 크기, 아이콘, 수량을 한 번에 받기.
	// 장비칸은 ItemType == Weapon만 구분.
FIntPoint UBaruInventoryWidget::GetGridDimensions() const
{
	if (!IsValid(InventoryComponent))
	{
		return FIntPoint::ZeroValue;
	}

	return FIntPoint(
		InventoryComponent->GridWidth,
		InventoryComponent->GridHeight);
}

TArray<FBaruInventoryUIEntry>
UBaruInventoryWidget::GetInventoryViewEntries() const
{
	TArray<FBaruInventoryUIEntry> Result;

	if (!IsValid(InventoryComponent)
		|| !IsValid(InventoryComponent->ItemDataTable))
	{
		return Result;
	}

	const TArray<FInventorySlot>& InventorySlots =
		InventoryComponent->GetSlots();

	Result.Reserve(InventorySlots.Num());

	for (const FInventorySlot& InventorySlot : InventorySlots)
	{
		if (InventorySlot.bEquipped	|| !IsValid(InventorySlot.Item))
		{
			continue;
		}

		const FItemData* ItemData =
			InventoryComponent->ItemDataTable->FindRow<FItemData>(
				InventorySlot.Item->ItemID,
				TEXT("Inventory UI"));

		if (!ItemData)
		{
			continue;
		}

		FBaruInventoryUIEntry& Entry =
			Result.AddDefaulted_GetRef();

		Entry.Item = InventorySlot.Item;
		Entry.ItemID = InventorySlot.Item->ItemID;
		Entry.ItemName = ItemData->ItemName;
		Entry.Thumbnail = ItemData->Thumbnail;
		Entry.TopLeft = InventorySlot.TopLeft;
		Entry.GridSize = ItemData->GridSize;
		Entry.Quantity = InventorySlot.Item->Quantity;
		Entry.ItemType = ItemData->ItemType;
	}

	return Result;
}

void UBaruInventoryWidget::RebuildEmptyGrid()
{
	if (!IsValid(InventoryGrid)
		|| !IsValid(InventorySizeBox)
		|| !InventoryCellWidgetClass)
	{
		return;
	}

	const FIntPoint GridDimensions = GetGridDimensions();

	if (GridDimensions.X <= 0 || GridDimensions.Y <= 0)
	{
		return;
	}

	InventoryGrid->ClearChildren();

	// 7열 × 10행이면 448×640 크기가 됩니다.
	InventorySizeBox->SetWidthOverride(
		static_cast<float>(GridDimensions.X) * GridCellSize);

	InventorySizeBox->SetHeightOverride(
		static_cast<float>(GridDimensions.Y) * GridCellSize);

	InventoryGrid->SetMinDesiredSlotWidth(GridCellSize);
	InventoryGrid->SetMinDesiredSlotHeight(GridCellSize);

	// Y가 행, X가 열입니다.
	for (int32 Y = 0; Y < GridDimensions.Y; ++Y)
	{
		for (int32 X = 0; X < GridDimensions.X; ++X)
		{
			UBaruInventoryCellWidget* CellWidget =
				CreateWidget<UBaruInventoryCellWidget>(
					GetOwningPlayer(),
					InventoryCellWidgetClass);

			if (!IsValid(CellWidget))
			{
				continue;
			}

			CellWidget->InitializeCell(FIntPoint(X, Y));

			UUniformGridSlot* GridSlot =
				InventoryGrid->AddChildToUniformGrid(
					CellWidget,
					Y,
					X);

			if (IsValid(GridSlot))
			{
				GridSlot->SetHorizontalAlignment(HAlign_Fill);
				GridSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
	}
}


void UBaruInventoryWidget::RebuildItemWidgets()
{
	if (!IsValid(InventoryItemCanvas)
		|| !InventoryItemWidgetClass)
	{
		return;
	}

	InventoryItemCanvas->ClearChildren();

	const TArray<FBaruInventoryUIEntry> Entries =
		GetInventoryViewEntries();

	for (const FBaruInventoryUIEntry& Entry : Entries)
	{
		if (!IsValid(Entry.Item))
		{
			continue;
		}

		UBaruInventoryItemWidget* ItemWidget =
			CreateWidget<UBaruInventoryItemWidget>(
				GetOwningPlayer(),
				InventoryItemWidgetClass);

		if (!IsValid(ItemWidget))
		{
			continue;
		}

		ItemWidget->InitializeItem(
			InventoryComponent,
			Entry.Item,
			Entry.Thumbnail,
			Entry.ItemName,
			Entry.Quantity);

		UCanvasPanelSlot* ItemCanvasSlot =
			InventoryItemCanvas->AddChildToCanvas(ItemWidget);

		if (!IsValid(ItemCanvasSlot))
		{
			continue;
		}

		const FVector2D ItemPosition(
			static_cast<float>(Entry.TopLeft.X) * GridCellSize,
			static_cast<float>(Entry.TopLeft.Y) * GridCellSize);

		const FVector2D ItemSize(
			static_cast<float>(Entry.GridSize.X) * GridCellSize,
			static_cast<float>(Entry.GridSize.Y) * GridCellSize);

		ItemCanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		ItemCanvasSlot->SetAlignment(FVector2D::ZeroVector);
		ItemCanvasSlot->SetPosition(ItemPosition);
		ItemCanvasSlot->SetSize(ItemSize);
		ItemCanvasSlot->SetAutoSize(false);
		ItemCanvasSlot->SetZOrder(1);
	}
}