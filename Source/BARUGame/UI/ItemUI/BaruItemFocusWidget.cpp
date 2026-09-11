#include "UI/ItemUI/BaruItemFocusWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Gameplay/Items/BaruBaseItem.h"
#include "Gameplay/Items/DataTypes/BaruItemData.h"
#include "Interfaces/InteractableInterface.h"

namespace
{
    FText ItemCategory(EItemType Type)
    {
        switch (Type)
        {
        case EItemType::Weapon: return NSLOCTEXT("BaruItemFocus", "Weapon", "무기");
        case EItemType::Bullet: return NSLOCTEXT("BaruItemFocus", "Bullet", "탄약");
        case EItemType::Package: return NSLOCTEXT("BaruItemFocus", "Package", "물품");
        case EItemType::Goods: return NSLOCTEXT("BaruItemFocus", "Goods", "잡화");
        case EItemType::Consumable: return NSLOCTEXT("BaruItemFocus", "Consumable", "소모품");
        case EItemType::Note: return NSLOCTEXT("BaruItemFocus", "Note", "기록물");
        default: return NSLOCTEXT("BaruItemFocus", "Item", "아이템");
        }
    }
}

TSharedRef<SWidget> UBaruItemFocusWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(
            UCanvasPanel::StaticClass(), TEXT("FocusRoot"));
        WidgetTree->RootWidget = Root;
        Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

        Card = WidgetTree->ConstructWidget<UBorder>(
            UBorder::StaticClass(), TEXT("FocusCard"));
        Card->SetBrush(FSlateRoundedBoxBrush(
            FLinearColor(0.025f, 0.035f, 0.045f, 0.97f), 9.0f,
            FLinearColor(0.65f, 0.42f, 0.09f, 1.0f), 1.5f, FVector2D(440, 154)));
        Card->SetPadding(FMargin(22.0f, 18.0f));
        Card->SetVisibility(ESlateVisibility::Collapsed);
        
        CardSlot = Root->AddChildToCanvas(Card);
        CardSlot->SetAnchors(FAnchors(0.5f, 0.78f));
        CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        CardSlot->SetPosition(FVector2D::ZeroVector);
        CardSlot->SetSize(FVector2D(440, 154));

        UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
        Card->SetContent(Column);
        auto MakeText = [this](FName Name, int32 FontSize, FLinearColor Color)
        {
            UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(
                UTextBlock::StaticClass(), Name);
            FSlateFontInfo Font = Text->GetFont();
            Font.Size = FontSize;
            Text->SetFont(Font);
            Text->SetColorAndOpacity(FSlateColor(Color));
            Text->SetClipping(EWidgetClipping::ClipToBounds);
            return Text;
        };

        UHorizontalBox* Top = WidgetTree->ConstructWidget<UHorizontalBox>();
        Column->AddChildToVerticalBox(Top);
        NameText = MakeText(TEXT("ItemName"), 21, FLinearColor::White);
        ValueText = MakeText(TEXT("ItemValue"), 21, FLinearColor(1.0f, 0.65f, 0.13f));
        UHorizontalBoxSlot* NameSlot = Top->AddChildToHorizontalBox(NameText);
        NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        NameSlot->SetPadding(FMargin(0, 0, 12, 0));
        Top->AddChildToHorizontalBox(ValueText);

        CategoryText = MakeText(TEXT("ItemCategory"), 13, FLinearColor(0.25f, 0.58f, 0.75f));
        UVerticalBoxSlot* CategorySlot = Column->AddChildToVerticalBox(CategoryText);
        CategorySlot->SetPadding(FMargin(0, 12, 0, 12));
        CategorySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

        UHorizontalBox* Bottom = WidgetTree->ConstructWidget<UHorizontalBox>();
        Column->AddChildToVerticalBox(Bottom);
        DetailText = MakeText(TEXT("ItemDetails"), 13, FLinearColor(0.60f, 0.70f, 0.77f));
        UHorizontalBoxSlot* DetailSlot = Bottom->AddChildToHorizontalBox(DetailText);
        DetailSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        DetailSlot->SetVerticalAlignment(VAlign_Center);

        UBorder* PromptBox = WidgetTree->ConstructWidget<UBorder>();
        PromptBox->SetBrush(FSlateRoundedBoxBrush(
            FLinearColor(0.65f, 0.035f, 0.06f), 5.0f, FVector2D(100, 30)));
        PromptBox->SetPadding(FMargin(15, 6));
        PromptText = MakeText(TEXT("PickupPrompt"), 13, FLinearColor::White);
        PromptBox->SetContent(PromptText);
        Bottom->AddChildToHorizontalBox(PromptBox);
    }
    SetVisibility(ESlateVisibility::HitTestInvisible);
    return Super::RebuildWidget();
}

void UBaruItemFocusWidget::ConfigureCard(FVector2D Anchor, float Width)
{
    if (CardSlot)
    {
        CardSlot->SetAnchors(FAnchors(
            FMath::Clamp(Anchor.X, 0.0, 1.0), FMath::Clamp(Anchor.Y, 0.0, 1.0)));
        CardSlot->SetSize(FVector2D(FMath::Clamp(Width, 320.0f, 800.0f), 154));
    }
}

void UBaruItemFocusWidget::HideItem()
{
    if (Card) Card->SetVisibility(ESlateVisibility::Collapsed);
}

void UBaruItemFocusWidget::ShowItem(const ABaruBaseItem* Item, APawn* Viewer)
{
    if (!Card || !IsValid(Item) || Item->PickupCount <= 0
        || !IsValid(Item->ItemRow.DataTable)
        || Item->ItemRow.DataTable->GetRowStruct() != FItemData::StaticStruct())
    {
        HideItem();
        return;
    }

    const FItemData* Data = Item->ItemRow.DataTable->FindRow<FItemData>(
        Item->ItemRow.RowName, TEXT("ItemFocusUI"), false);
    if (!Data)
    {
        HideItem();
        return;
    }

    const FText Name = Data->ItemName.IsEmpty()
        ? FText::FromName(Item->ItemRow.RowName) : Data->ItemName;
    NameText->SetText(Item->PickupCount > 1
        ? FText::Format(NSLOCTEXT("BaruItemFocus", "StackName", "{0} ×{1}"),
            Name, FText::AsNumber(Item->PickupCount)) : Name);

    const int64 Value = static_cast<int64>(FMath::Max(0, Data->SettlementValuePerUnit))
        * static_cast<int64>(Item->PickupCount);
    ValueText->SetText(FText::Format(NSLOCTEXT("BaruItemFocus", "Price", "{0}P"),
        FText::AsNumber(Value)));
    CategoryText->SetText(Data->PickupCategoryText.IsEmpty()
        ? ItemCategory(Data->ItemType) : Data->PickupCategoryText);

    const int64 Cells = static_cast<int64>(FMath::Max(1, Data->GridSize.X))
        * static_cast<int64>(FMath::Max(1, Data->GridSize.Y));
    const double Weight = FMath::IsFinite(Data->UnitWeightKg)
        ? static_cast<double>(FMath::Max(0.0f, Data->UnitWeightKg)) * Item->PickupCount
        : 0.0;
    FNumberFormattingOptions WeightFormat;
    WeightFormat.SetMinimumFractionalDigits(1);
    WeightFormat.SetMaximumFractionalDigits(1);
    DetailText->SetText(FText::Format(
        NSLOCTEXT("BaruItemFocus", "Details", "{0}칸 / {1}kg"),
        FText::AsNumber(Cells), FText::AsNumber(Weight, &WeightFormat)));
    PromptText->SetText(IInteractableInterface::Execute_GetInteractPromptText(Item, Viewer));
    Card->SetVisibility(ESlateVisibility::HitTestInvisible);
}
