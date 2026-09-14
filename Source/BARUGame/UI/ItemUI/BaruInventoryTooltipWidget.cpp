// BaruInventoryTooltipWidget.cpp


#include "UI/ItemUI/BaruInventoryTooltipWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "UObject/UnrealType.h"

void UBaruInventoryTooltipWidget::InitializeInfo(
    const FItemData& InData,
    int32 InQuantity)
{
    DisplayData = InData;
    DisplayQuantity = FMath::Max(0, InQuantity);

    RefreshInfo();
}

TSharedRef<SWidget> UBaruInventoryTooltipWidget::RebuildWidget()
{
    if (WidgetTree && !WidgetTree->RootWidget)
    {
        USizeBox* Root =
            WidgetTree->ConstructWidget<USizeBox>();

        Root->SetWidthOverride(360.0f);
        WidgetTree->RootWidget = Root;

        UBorder* Background =
            WidgetTree->ConstructWidget<UBorder>();

        Background->SetBrushColor(
            FLinearColor(0.025f, 0.035f, 0.045f, 0.98f));
        Background->SetPadding(FMargin(16.0f));
        Root->SetContent(Background);

        UVerticalBox* Column =
            WidgetTree->ConstructWidget<UVerticalBox>();
        Background->SetContent(Column);

        auto MakeText = [this](
            int32 FontSize,
            const FLinearColor& Color) -> UTextBlock*
        {
            UTextBlock* Text =
                WidgetTree->ConstructWidget<UTextBlock>();

            FSlateFontInfo Font = Text->GetFont();
            Font.Size = FontSize;

            Text->SetFont(Font);
            Text->SetColorAndOpacity(FSlateColor(Color));
            Text->SetWrapTextAt(328.0f);

            return Text;
        };

        UHorizontalBox* Header =
            WidgetTree->ConstructWidget<UHorizontalBox>();
        Column->AddChildToVerticalBox(Header);

        USizeBox* IconBox =
            WidgetTree->ConstructWidget<USizeBox>();
        IconBox->SetWidthOverride(64.0f);
        IconBox->SetHeightOverride(64.0f);

        Image_Icon = WidgetTree->ConstructWidget<UImage>();
        IconBox->SetContent(Image_Icon);
        Header->AddChildToHorizontalBox(IconBox);

        Text_Name = MakeText(20, FLinearColor::White);
        Text_Name->SetWrapTextAt(252.0f);

        UHorizontalBoxSlot* NameSlot =
            Header->AddChildToHorizontalBox(Text_Name);
        NameSlot->SetPadding(FMargin(12, 0, 0, 0));
        NameSlot->SetVerticalAlignment(VAlign_Center);
        NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

        Text_Info = MakeText(
            14, FLinearColor(0.75f, 0.83f, 0.90f));
        Column->AddChildToVerticalBox(Text_Info)
            ->SetPadding(FMargin(0, 12, 0, 0));

        Text_Description = MakeText(
            14, FLinearColor(0.95f, 0.95f, 0.95f));
        Column->AddChildToVerticalBox(Text_Description)
            ->SetPadding(FMargin(0, 12, 0, 0));

        Text_Story = MakeText(
            13, FLinearColor(0.75f, 0.65f, 0.44f));
        Column->AddChildToVerticalBox(Text_Story)
            ->SetPadding(FMargin(0, 12, 0, 0));
    }

    RefreshInfo();
    return Super::RebuildWidget();
}

void UBaruInventoryTooltipWidget::RefreshInfo()
{
    if (!IsValid(Text_Name))
    {
        return;
    }

    Text_Name->SetText(DisplayData.ItemName.IsEmpty()
        ? FText::FromName(DisplayData.ItemID)
        : DisplayData.ItemName);

    Image_Icon->SetBrushFromTexture(DisplayData.Thumbnail);
    Image_Icon->SetVisibility(IsValid(DisplayData.Thumbnail)
        ? ESlateVisibility::HitTestInvisible
        : ESlateVisibility::Collapsed);

    const FText Category = DisplayData.PickupCategoryText.IsEmpty()
        ? StaticEnum<EItemType>()->GetDisplayNameTextByValue(
            static_cast<int64>(DisplayData.ItemType))
        : DisplayData.PickupCategoryText;

    const FText Rarity =
        StaticEnum<EBaruItemRarity>()->GetDisplayNameTextByValue(
            static_cast<int64>(DisplayData.Rarity));

    const double UnitWeight =
        FMath::IsFinite(DisplayData.UnitWeightKg)
        ? FMath::Max(0.0, static_cast<double>(DisplayData.UnitWeightKg))
        : 0.0;

    const int64 UnitValue =
        FMath::Max(0, DisplayData.SettlementValuePerUnit);

    FNumberFormattingOptions WeightFormat;
    WeightFormat.SetMinimumFractionalDigits(1);
    WeightFormat.SetMaximumFractionalDigits(1);

    Text_Info->SetText(FText::Format(
        NSLOCTEXT("BaruInventoryTooltip", "Info",
            "{0} · {1}\n"
            "크기: {2} × {3}칸 / 수량: {4}\n"
            "개당 무게: {5} kg / 총 무게: {6} kg\n"
            "개당 가치: {7}P / 총 가치: {8}P\n"
            "정산 포함: {9}"),
        Category,
        Rarity,
        FText::AsNumber(DisplayData.GridSize.X),
        FText::AsNumber(DisplayData.GridSize.Y),
        FText::AsNumber(DisplayQuantity),
        FText::AsNumber(UnitWeight, &WeightFormat),
        FText::AsNumber(UnitWeight * DisplayQuantity, &WeightFormat),
        FText::AsNumber(UnitValue),
        FText::AsNumber(UnitValue * DisplayQuantity),
        DisplayData.bCanBeSettled
            ? NSLOCTEXT("BaruInventoryTooltip", "Yes", "예")
            : NSLOCTEXT("BaruInventoryTooltip", "No", "아니오")));

    Text_Description->SetText(DisplayData.ItemDescription);
    Text_Description->SetVisibility(DisplayData.ItemDescription.IsEmpty()
        ? ESlateVisibility::Collapsed
        : ESlateVisibility::HitTestInvisible);

    Text_Story->SetText(DisplayData.ItemStory);
    Text_Story->SetVisibility(DisplayData.ItemStory.IsEmpty()
        ? ESlateVisibility::Collapsed
        : ESlateVisibility::HitTestInvisible);
}

void UBaruInventoryTooltipWidget::NativeTick(
    const FGeometry& MyGeometry,
    float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    UpdatePlacement();
}

void UBaruInventoryTooltipWidget::UpdatePlacement()
{
    APlayerController* PC = GetOwningPlayer();
    if (!IsValid(PC))
    {
        return;
    }

    const FGeometry ScreenGeometry =
        UWidgetLayoutLibrary::GetPlayerScreenWidgetGeometry(PC);

    const FVector2D ScreenSize = ScreenGeometry.GetLocalSize();

    constexpr double Margin = 8.0;
    constexpr double Gap = 16.0;

    if (ScreenSize.X <= Margin * 2 || ScreenSize.Y <= Margin * 2)
    {
        return;
    }

    ForceLayoutPrepass();

    const FVector2D NaturalSize = GetDesiredSize();
    if (NaturalSize.X <= 0 || NaturalSize.Y <= 0)
    {
        return;
    }

    // 설명이 길어도 카드 전체가 화면 안에 들어오도록 축소합니다.
    const double Scale = FMath::Min(
        1.0,
        FMath::Min(
            (ScreenSize.X - Margin * 2) / NaturalSize.X,
            (ScreenSize.Y - Margin * 2) / NaturalSize.Y));

    SetRenderTransformPivot(FVector2D::ZeroVector);
    SetRenderScale(FVector2D(Scale, Scale));
    SetDesiredSizeInViewport(NaturalSize);

    const FVector2D CardSize = NaturalSize * Scale;

    const FVector2D MousePosition =
        ScreenGeometry.AbsoluteToLocal(
            UWidgetLayoutLibrary::GetMousePositionOnPlatform());

    FVector2D Position = MousePosition + FVector2D(Gap, Gap);

    if (Position.X + CardSize.X > ScreenSize.X - Margin)
    {
        Position.X = MousePosition.X - CardSize.X - Gap;
    }

    if (Position.Y + CardSize.Y > ScreenSize.Y - Margin)
    {
        Position.Y = MousePosition.Y - CardSize.Y - Gap;
    }

    Position.X = FMath::Clamp(
        Position.X, Margin, ScreenSize.X - CardSize.X - Margin);

    Position.Y = FMath::Clamp(
        Position.Y, Margin, ScreenSize.Y - CardSize.Y - Margin);

    // 이미 플레이어 화면의 로컬 좌표이므로 DPI를 다시 나누지 않음.
    SetPositionInViewport(Position, false);
    SetRenderOpacity(1.0f);
}

