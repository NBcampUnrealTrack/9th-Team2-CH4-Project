// BaruCharacterEquipmentWidget.cpp

#include "UI/InventoryUI/BaruCharacterEquipmentWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Pawn.h"

#include "Player/BaruPlayerState.h"
#include "Gameplay/Equipment/BaruEquipmentComponent.h"
#include "Gameplay/Inventory/BaruInventoryComponent.h"
#include "Gameplay/Items/BaruItemInstance.h"
#include "Gameplay/Items/DataTypes/BaruItemData.h"

#include "InputCoreTypes.h"
#include "Input/Reply.h"

#include "Blueprint/WidgetBlueprintLibrary.h"   //[장비] 드래그 앤 드랍
#include "UI/InventoryUI/BaruInventoryDragDropOperation.h"
#include "Gameplay/Weapon/Data/BaruWeaponDataAsset.h"
#include "Components/Image.h"

#include "Components/Button.h"
#include "Gameplay/Equipment/DataTypes/BaruEquipmentTypes.h"



void UBaruCharacterEquipmentWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (IsValid(Button_PrimaryWeapon))
    {
        Button_PrimaryWeapon->OnClicked.AddUniqueDynamic(
            this,
            &UBaruCharacterEquipmentWidget::HandlePrimaryWeaponClicked);
    }

    if (IsValid(Button_SecondaryWeapon))
    {
        Button_SecondaryWeapon->OnClicked.AddUniqueDynamic(
            this,
            &UBaruCharacterEquipmentWidget::HandleSecondaryWeaponClicked);
    }

    BindEquipment();
    RefreshWeaponSlots();
}

void UBaruCharacterEquipmentWidget::NativeDestruct()
{
    if (IsValid(EquipmentComponent))
    {
        EquipmentComponent->OnEquipmentUpdated.RemoveAll(this);
    }

    EquipmentComponent = nullptr;
    InventoryComponent = nullptr;
    
    if (IsValid(Button_PrimaryWeapon))
    {
        Button_PrimaryWeapon->OnClicked.RemoveDynamic(
            this,
            &UBaruCharacterEquipmentWidget::HandlePrimaryWeaponClicked);
    }

    if (IsValid(Button_SecondaryWeapon))
    {
        Button_SecondaryWeapon->OnClicked.RemoveDynamic(
            this,
            &UBaruCharacterEquipmentWidget::HandleSecondaryWeaponClicked);
    }

    Super::NativeDestruct();
}

void UBaruCharacterEquipmentWidget::BindEquipment()
{
    if (IsValid(EquipmentComponent))
    {
        EquipmentComponent->OnEquipmentUpdated.RemoveAll(this);
    }

    EquipmentComponent = nullptr;
    InventoryComponent = nullptr;

    APawn* OwningPawn = GetOwningPlayerPawn();
    if (IsValid(OwningPawn))
    {
        EquipmentComponent =
            OwningPawn->FindComponentByClass<
                UBaruEquipmentComponent>();
    }

    ABaruPlayerState* BaruPlayerState =
        GetOwningPlayerState<ABaruPlayerState>();

    if (IsValid(BaruPlayerState))
    {
        InventoryComponent =
            BaruPlayerState->GetInventoryComponent();
    }

    if (IsValid(EquipmentComponent))
    {
        EquipmentComponent->OnEquipmentUpdated.AddUObject(
            this,
            &UBaruCharacterEquipmentWidget::
                HandleEquipmentUpdated);
    }
}

void UBaruCharacterEquipmentWidget::
HandleEquipmentUpdated()
{
    RefreshWeaponSlots();
}

void UBaruCharacterEquipmentWidget::RefreshWeaponSlots()
{
    UBaruItemInstance* PrimaryItem = nullptr;
    UBaruItemInstance* SecondaryItem = nullptr;

    if (IsValid(EquipmentComponent))
    {
        PrimaryItem =
            EquipmentComponent->GetEquippedWeaponItem(
                EBaruEquipmentSlot::PrimaryWeapon);

        SecondaryItem =
            EquipmentComponent->GetEquippedWeaponItem(
                EBaruEquipmentSlot::SecondaryWeapon);
    }

    SetWeaponSlotDisplay(
        Image_PrimaryWeapon,
        Text_PrimaryWeaponName,
        PrimaryItem);

    SetWeaponSlotDisplay(
        Image_SecondaryWeapon,
        Text_SecondaryWeaponName,
        SecondaryItem);

    const EBaruEquipmentSlot ActiveSlot =
        IsValid(EquipmentComponent)
        ? EquipmentComponent->GetActiveWeaponSlot()
        : EBaruEquipmentSlot::None;

    if (IsValid(Border_PrimaryWeaponSlot))
    {
        Border_PrimaryWeaponSlot->SetBrushColor(
            ActiveSlot == EBaruEquipmentSlot::PrimaryWeapon
            ? ActiveSlotColor
            : InactiveSlotColor);
    }

    if (IsValid(Border_SecondaryWeaponSlot))
    {
        Border_SecondaryWeaponSlot->SetBrushColor(
            ActiveSlot == EBaruEquipmentSlot::SecondaryWeapon
            ? ActiveSlotColor
            : InactiveSlotColor);
    }
}

void UBaruCharacterEquipmentWidget::SetWeaponSlotDisplay(
    UImage* TargetImage,
    UTextBlock* TargetNameText,
    UBaruItemInstance* EquippedItem)
{
    const FItemData* ItemData = nullptr;

    if (IsValid(EquippedItem)
        && IsValid(InventoryComponent)
        && IsValid(InventoryComponent->ItemDataTable))
    {
        ItemData =
            InventoryComponent->ItemDataTable
                ->FindRow<FItemData>(
                    EquippedItem->ItemID,
                    TEXT("CharacterEquipmentWidget"),
                    false);
    }

    if (!ItemData)
    {
        if (IsValid(TargetImage))
        {
            TargetImage->SetVisibility(
                ESlateVisibility::Collapsed);
        }

        if (IsValid(TargetNameText))
        {
            TargetNameText->SetText(
                FText::FromString(TEXT("비어 있음")));
        }

        return;
    }

    if (IsValid(TargetImage))
    {
        if (IsValid(ItemData->Thumbnail))
        {
            TargetImage->SetBrushFromTexture(
                ItemData->Thumbnail);

            TargetImage->SetVisibility(
                ESlateVisibility::HitTestInvisible);
        }
        else
        {
            TargetImage->SetVisibility(
                ESlateVisibility::Collapsed);
        }
    }

    if (IsValid(TargetNameText))
    {
        TargetNameText->SetText(ItemData->ItemName);
    }
}

void UBaruCharacterEquipmentWidget::HandlePrimaryWeaponClicked()
{
    if (IsValid(EquipmentComponent))
    {
        EquipmentComponent->RequestSetActiveWeaponSlot(
            EBaruEquipmentSlot::PrimaryWeapon);
    }
}

void UBaruCharacterEquipmentWidget::HandleSecondaryWeaponClicked()
{
    if (IsValid(EquipmentComponent))
    {
        EquipmentComponent->RequestSetActiveWeaponSlot(
            EBaruEquipmentSlot::SecondaryWeapon);
    }
}

    //우클릭, 더블클릭 장비해제.
EBaruEquipmentSlot
UBaruCharacterEquipmentWidget::FindWeaponSlotUnderMouse(
    const FVector2D& ScreenPosition) const
{
    if (IsValid(Border_PrimaryWeaponSlot)
        && Border_PrimaryWeaponSlot
            ->GetCachedGeometry()
            .IsUnderLocation(ScreenPosition))
    {
        return EBaruEquipmentSlot::PrimaryWeapon;
    }

    if (IsValid(Border_SecondaryWeaponSlot)
        && Border_SecondaryWeaponSlot
            ->GetCachedGeometry()
            .IsUnderLocation(ScreenPosition))
    {
        return EBaruEquipmentSlot::SecondaryWeapon;
    }

    return EBaruEquipmentSlot::None;
}

bool UBaruCharacterEquipmentWidget::RequestUnequipSlot(
    EBaruEquipmentSlot WeaponSlot)
{
    if (!IsValid(EquipmentComponent)
        || WeaponSlot == EBaruEquipmentSlot::None
        || !IsValid(
            EquipmentComponent->GetEquippedWeaponItem(
                WeaponSlot)))
    {
        return false;
    }

    EquipmentComponent->RequestUnequipWeapon(WeaponSlot);
    return true;
}

FReply UBaruCharacterEquipmentWidget::NativeOnMouseButtonDown(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton()
        == EKeys::RightMouseButton)
    {
        const EBaruEquipmentSlot WeaponSlot =
            FindWeaponSlotUnderMouse(
                InMouseEvent.GetScreenSpacePosition());

        if (RequestUnequipSlot(WeaponSlot))
        {
            return FReply::Handled();
        }
    }

    return Super::NativeOnMouseButtonDown(
        InGeometry,
        InMouseEvent);
}

FReply UBaruCharacterEquipmentWidget::
NativeOnMouseButtonDoubleClick(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton()
        == EKeys::LeftMouseButton)
    {
        const EBaruEquipmentSlot WeaponSlot =
            FindWeaponSlotUnderMouse(
                InMouseEvent.GetScreenSpacePosition());

        if (RequestUnequipSlot(WeaponSlot))
        {
            return FReply::Handled();
        }
    }

    return Super::NativeOnMouseButtonDoubleClick(
        InGeometry,
        InMouseEvent);
}

    //[장비]드래그 앤 드롭
FReply UBaruCharacterEquipmentWidget::
NativeOnPreviewMouseButtonDown(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton()
        == EKeys::LeftMouseButton)
    {
        const EBaruEquipmentSlot WeaponSlot =
            FindWeaponSlotUnderMouse(
                InMouseEvent.GetScreenSpacePosition());

        if (WeaponSlot != EBaruEquipmentSlot::None
            && IsValid(EquipmentComponent)
            && IsValid(
                EquipmentComponent->GetEquippedWeaponItem(
                    WeaponSlot)))
        {
            PendingWeaponDragSlot = WeaponSlot;

            return FReply::Handled()
                .CaptureMouse(TakeWidget())
                .DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
        }
    }

    return Super::NativeOnPreviewMouseButtonDown(
        InGeometry,
        InMouseEvent);
}

FReply UBaruCharacterEquipmentWidget::NativeOnMouseButtonUp(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
        && PendingWeaponDragSlot != EBaruEquipmentSlot::None)
    {
        const EBaruEquipmentSlot SelectedSlot =
            PendingWeaponDragSlot;

        PendingWeaponDragSlot = EBaruEquipmentSlot::None;

        const EBaruEquipmentSlot ReleasedSlot =
            FindWeaponSlotUnderMouse(
                InMouseEvent.GetScreenSpacePosition());

        // 다른 곳에서 마우스를 놓았으면 무기를 전환하지 않습니다.
        if (ReleasedSlot == SelectedSlot
            && IsValid(EquipmentComponent))
        {
            EquipmentComponent->RequestSetActiveWeaponSlot(
                SelectedSlot);
        }

        return FReply::Handled().ReleaseMouseCapture();
    }

    return Super::NativeOnMouseButtonUp(
        InGeometry,
        InMouseEvent);
}

void UBaruCharacterEquipmentWidget::NativeOnDragDetected(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent,
    UDragDropOperation*& OutOperation)
{
    Super::NativeOnDragDetected(
        InGeometry,
        InMouseEvent,
        OutOperation);

    const EBaruEquipmentSlot DraggedSlot =
        PendingWeaponDragSlot;

    PendingWeaponDragSlot =
        EBaruEquipmentSlot::None;

    if (DraggedSlot == EBaruEquipmentSlot::None
        || !IsValid(EquipmentComponent))
    {
        return;
    }

    UBaruItemInstance* EquippedItem =
        EquipmentComponent->GetEquippedWeaponItem(
            DraggedSlot);

    if (!IsValid(EquippedItem))
    {
        return;
    }

    UBaruInventoryDragDropOperation* DragOperation =
        NewObject<UBaruInventoryDragDropOperation>(this);

    DragOperation->ItemInstance = EquippedItem;
    DragOperation->SourceEquipmentSlot = DraggedSlot;
    // 커서 위치를 드래그 이미지의 좌상단으로 사용.
    // Grid 반환 코드도 커서 위치를 아이템의 좌상단 칸으로 사용합니다.
    DragOperation->Pivot = EDragPivot::TopLeft;
    DragOperation->Offset = FVector2D::ZeroVector;

    // 장비 아이콘을 드래그 이미지로 표시합니다.
    UImage* SourceImage =
    DraggedSlot == EBaruEquipmentSlot::PrimaryWeapon
    ? Image_PrimaryWeapon.Get()
    : Image_SecondaryWeapon.Get();

    if (IsValid(SourceImage))
    {
        UImage* DragVisual = NewObject<UImage>(DragOperation);

        FSlateBrush DragBrush = SourceImage->GetBrush();
        DragBrush.SetImageSize(FVector2D(96.0f, 96.0f));
        DragBrush.DrawAs = ESlateBrushDrawType::Image;

        DragVisual->SetBrush(DragBrush);
        DragVisual->SetVisibility(
            ESlateVisibility::HitTestInvisible);

        DragOperation->DefaultDragVisual = DragVisual;
    }

    OutOperation = DragOperation;
}


    //[장비] 드래그 앤 드랍
bool UBaruCharacterEquipmentWidget::NativeOnDrop(
    const FGeometry& InGeometry,
    const FDragDropEvent& InDragDropEvent,
    UDragDropOperation* InOperation)
{
    UBaruInventoryDragDropOperation* DragOperation =
        Cast<UBaruInventoryDragDropOperation>(InOperation);

    if (!IsValid(DragOperation)
        || !IsValid(DragOperation->ItemInstance)
        || !IsValid(InventoryComponent)
        || !IsValid(InventoryComponent->ItemDataTable))
    {
        return false;
    }

    // 장비칸에서 시작한 드래그는 여기서 처리하지 않습니다.
    if (DragOperation->SourceEquipmentSlot
        != EBaruEquipmentSlot::None)
    {
        return false;
    }

    const EBaruEquipmentSlot TargetSlot =
        FindWeaponSlotUnderMouse(
            InDragDropEvent.GetScreenSpacePosition());

    if (TargetSlot == EBaruEquipmentSlot::None)
    {
        return false;
    }

    const FItemData* ItemData =
        InventoryComponent->ItemDataTable->FindRow<FItemData>(
            DragOperation->ItemInstance->ItemID,
            TEXT("EquipmentDrop"),
            false);

    if (!ItemData
        || ItemData->ItemType != EItemType::Weapon
        || ItemData->WeaponDataAsset.IsNull())
    {
        return false;
    }

    const UBaruWeaponDataAsset* WeaponData =
        ItemData->WeaponDataAsset.LoadSynchronous();

    // 드롭한 장비칸과 무기의 장착 슬롯이 맞는지 확인.
    if (!IsValid(WeaponData)
        || WeaponData->EquipmentSlot != TargetSlot)
    {
        return false;
    }

    InventoryComponent->RequestUseItem(
        DragOperation->ItemInstance);

    // 드롭 요청을 처리했다는 뜻이며,
    // 실제 장착 성공 여부는 서버에서 결정합니다.
    return true;
}