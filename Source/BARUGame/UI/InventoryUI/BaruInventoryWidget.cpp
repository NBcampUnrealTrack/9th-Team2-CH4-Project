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

#include "UI/InventoryUI/BaruInventoryDragDropOperation.h"

#include "Gameplay/Equipment/BaruEquipmentComponent.h"	//[장비] 드래그 앤 드랍.
#include "GameFramework/Pawn.h"

#include "Components/CanvasPanel.h"	// 키입력 구현부.
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "BaruLog.h"





void UBaruInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindInventory();

	// 실제 InventoryComponent의 7×10 설정으로 빈 칸을 생성.
	RebuildEmptyGrid();

	RebuildItemWidgets();

	// 그 위에 실제 아이템을 그리도록 BP에 알림.
	RefreshInventoryView();
	RefreshWeightDisplay();
	if (!IsValid(Text_Weight))
	{
		BARU_LOG(LogBaruUI, Warning,
			TEXT("Inventory weight: WBP_BaruInventoryGrid 안의 TextBlock 이름을 Text_Weight로 지정해주세요."));
	}
		// PlayerState/아이템 수량이 UI보다 늦게 복제되는 경우도 보정.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(WeightRefreshTimer, this,
			&UBaruInventoryWidget::RefreshInventoryBindingAndWeight, 0.25f, true);
	}
}




void UBaruInventoryWidget::NativeDestruct()
{	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WeightRefreshTimer);
	}
	LastWeightText = FText::GetEmpty();
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
	RefreshWeightDisplay();
}

void UBaruInventoryWidget::RefreshWeightDisplay()
{
	if (!IsValid(Text_Weight)) return;
	FText Display;
	if (IsValid(InventoryComponent))
	{
		FNumberFormattingOptions Options;
		Options.SetMinimumFractionalDigits(1);
		Options.SetMaximumFractionalDigits(1);
		Display = FText::Format(NSLOCTEXT("BaruInventory", "TotalWeight", "무게: {0} kg"),
			FText::AsNumber(InventoryComponent->GetTotalCarriedWeightKg(), &Options));
	}
	else
	{
		Display = NSLOCTEXT("BaruInventory", "WeightWaiting", "무게: -- kg");
	}
	if (!Display.EqualTo(LastWeightText) || !Text_Weight->GetText().EqualTo(Display))
	{
		Text_Weight->SetText(Display);
		LastWeightText = Display;
	}
}

void UBaruInventoryWidget::RefreshInventoryBindingAndWeight()
{
	const ABaruPlayerState* PS = GetOwningPlayerState<ABaruPlayerState>();
	UBaruInventoryComponent* Current = IsValid(PS) ? PS->GetInventoryComponent() : nullptr;
	if (InventoryComponent != Current)
	{
		BindInventory();
		RebuildEmptyGrid();
		HandleInventoryUpdated();
	}
	else
	{
		RefreshWeightDisplay();
	}
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


bool UBaruInventoryWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	UBaruInventoryDragDropOperation* DragOperation =
		Cast<UBaruInventoryDragDropOperation>(InOperation);

	if (!IsValid(DragOperation)
		|| !IsValid(DragOperation->ItemInstance)
		|| !IsValid(InventoryComponent)
		|| !IsValid(InventoryItemCanvas))
	{
		return false;
	}

	const FVector2D ScreenPosition =
		InDragDropEvent.GetScreenSpacePosition();

	const FGeometry& CanvasGeometry =
		InventoryItemCanvas->GetCachedGeometry();

	if (!CanvasGeometry.IsUnderLocation(ScreenPosition))
	{
		return false;
	}

	const FVector2D LocalPosition =
		CanvasGeometry.AbsoluteToLocal(ScreenPosition);

	const FIntPoint TargetCell(
		FMath::FloorToInt(LocalPosition.X / GridCellSize),
		FMath::FloorToInt(LocalPosition.Y / GridCellSize));
	
		//[장비] 드래그 앤 드랍
	// 장비칸에서 가져온 무기라면 장비를 해제하고 인벤토리로 반환합니다.
	if (DragOperation->SourceEquipmentSlot
		!= EBaruEquipmentSlot::None)
	{
		APawn* OwningPawn = GetOwningPlayerPawn();

		UBaruEquipmentComponent* EquipmentComponent =
			IsValid(OwningPawn)
			? OwningPawn->FindComponentByClass<
				UBaruEquipmentComponent>()
			: nullptr;

		if (!IsValid(EquipmentComponent))
		{
			return false;
		}

		EquipmentComponent->RequestUnequipWeaponAtCell(
			DragOperation->SourceEquipmentSlot,
			TargetCell);

		return true;
	}
	
		//인벤토리 안에서 시작한 드래그.
	InventoryComponent->RequestMoveItem(
		DragOperation->ItemInstance,
		TargetCell);

	return true;
}
