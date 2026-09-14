#include "UI/InventoryUI/BaruInventoryScreenWidget.h"
#include "Components/SizeBox.h"
#include "Player/BaruPlayerState.h"
#include "Player/BaruPlayerController.h"
#include "Gameplay/Inventory/BaruInventoryComponent.h"
#include "Gameplay/Items/BaruItemInstance.h"
#include "UI/InventoryUI/BaruInventoryDragDropOperation.h"
#include "Input/Reply.h"

UBaruInventoryScreenWidget::UBaruInventoryScreenWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputConfig = EBaruWidgetInputMode::GameAndMenu;
	GameMouseCaptureMode = EMouseCaptureMode::NoCapture;

	// [수정 09.14] 슬레이트 키 입력을 받을 수 있도록 설정
	SetIsFocusable(true);
	bSupportsActivationFocus = true;
	bIsBackHandler = false;
}

// [09.14] 인벤토리 화면 어디를 클릭해서 UI 포커스가 잡혀 있어도 I 키를 누르면 닫힘
FReply UBaruInventoryScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::I)
	{
		if (ABaruPlayerController* PC = Cast<ABaruPlayerController>(GetOwningPlayer()))
		{
			PC->ToggleInventory();
			return FReply::Handled();
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

bool UBaruInventoryScreenWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	UBaruInventoryDragDropOperation* DragOperation =
		Cast<UBaruInventoryDragDropOperation>(InOperation);
	if (!IsValid(DragOperation) || !IsValid(DragOperation->ItemInstance)
		|| !IsValid(SizeBox_Content))
	{
		return false;
	}

	const FVector2D CursorPosition =
		InDragDropEvent.GetScreenSpacePosition();

	// 자식 Grid/장비칸에서 거절된 드롭이 여기까지 전달돼도 버리지 않습니다.
	if (SizeBox_Content->GetCachedGeometry().IsUnderLocation(CursorPosition))
	{
		return true;
	}

	if (!InGeometry.IsUnderLocation(CursorPosition))
	{
		return false;
	}

	ABaruPlayerState* OwnerPS = GetOwningPlayerState<ABaruPlayerState>();
	UBaruInventoryComponent* Inventory =
		IsValid(OwnerPS) ? OwnerPS->GetInventoryComponent() : nullptr;
	if (!IsValid(Inventory))
	{
		return false;
	}

	// 아이템 소유권, 생성 위치, 수량은 Inventory의 서버 처리에서 재검사합니다.
	Inventory->RequestDropEntireItem(DragOperation->ItemInstance);
	return true;
}