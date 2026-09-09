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