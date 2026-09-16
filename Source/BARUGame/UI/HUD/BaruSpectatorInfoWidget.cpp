#include "UI/HUD/BaruSpectatorInfoWidget.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Player/BaruPlayerController.h"

void UBaruSpectatorInfoWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UpdateSpectatorDisplay(TEXT(""));
    InitControllerBinding();
}

void UBaruSpectatorInfoWidget::NativeDestruct()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(RetryInitTimerHandle);
    }

    if (CachedPlayerController.IsValid())
    {
        CachedPlayerController->OnSpectatorTargetChanged.RemoveDynamic(this, &UBaruSpectatorInfoWidget::HandleSpectatorTargetChanged);
    }

    CachedPlayerController.Reset();
    Super::NativeDestruct();
}

void UBaruSpectatorInfoWidget::InitControllerBinding()
{
    APlayerController* PC = GetOwningPlayer();
    if (ABaruPlayerController* BaruPC = Cast<ABaruPlayerController>(PC))
    {
        CachedPlayerController = BaruPC;
        BaruPC->OnSpectatorTargetChanged.AddUniqueDynamic(this, &UBaruSpectatorInfoWidget::HandleSpectatorTargetChanged);
        return;
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(RetryInitTimerHandle, this, &UBaruSpectatorInfoWidget::InitControllerBinding, 0.1f, false);
    }
}

void UBaruSpectatorInfoWidget::HandleSpectatorTargetChanged(const FString& TargetPlayerName)
{
    UpdateSpectatorDisplay(TargetPlayerName);
}

void UBaruSpectatorInfoWidget::UpdateSpectatorDisplay(const FString& TargetPlayerName)
{
    if (TargetPlayerName.IsEmpty())
    {
        SetVisibility(ESlateVisibility::Collapsed);
        if (Border_SpectatorInfo)
        {
            Border_SpectatorInfo->SetVisibility(ESlateVisibility::Collapsed);
        }
        return;
    }

    SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    if (Border_SpectatorInfo)
    {
        Border_SpectatorInfo->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }

    if (Text_SpectatorTargetName)
    {
        Text_SpectatorTargetName->SetText(FText::Format(
            NSLOCTEXT("BaruHUD", "SpectatingFormat", "관전 중: {0}"),
            FText::FromString(TargetPlayerName)
        ));
    }

    if (Text_SpectatorGuide)
    {
        Text_SpectatorGuide->SetText(FText::FromString(TEXT("[ 좌클릭 / 우클릭 ] 대상 전환")));
    }
}