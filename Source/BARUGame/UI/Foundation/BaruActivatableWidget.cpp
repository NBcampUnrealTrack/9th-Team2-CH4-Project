// BaruActivatableWidget.cpp

#include "UI/Foundation/BaruActivatableWidget.h"

#include "CommonInputModeTypes.h"
#include "Input/UIActionBindingHandle.h"
#include "GameFramework/PlayerController.h"

TOptional<FUIInputConfig>
UBaruActivatableWidget::GetDesiredInputConfig() const
{
	switch (InputConfig)
	{
	case EBaruWidgetInputMode::GameAndMenu:
		return FUIInputConfig(
			ECommonInputMode::All,
			GameMouseCaptureMode);

	case EBaruWidgetInputMode::Game:
		return FUIInputConfig(
			ECommonInputMode::Game,
			GameMouseCaptureMode);

	case EBaruWidgetInputMode::Menu:
		return FUIInputConfig(
			ECommonInputMode::Menu,
			EMouseCaptureMode::NoCapture);

	case EBaruWidgetInputMode::Default:
	default:
		// 값이 없으면 Common UI가 현재 입력 설정을
		// 강제로 변경하지 않는다.
		return TOptional<FUIInputConfig>();
	}
}

void UBaruActivatableWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	APlayerController* OwningPlayerController =
		GetOwningPlayer();

	if (!IsValid(OwningPlayerController))
	{
		return;
	}

	bPreviousMouseCursorVisible =
		OwningPlayerController->bShowMouseCursor;

	bHasSavedMouseCursorState = true;

	const bool bNeedsMouseCursor =
		InputConfig == EBaruWidgetInputMode::Menu
		|| InputConfig == EBaruWidgetInputMode::GameAndMenu;

	if (bNeedsMouseCursor)
	{
		OwningPlayerController->SetShowMouseCursor(true);
	}
}

void UBaruActivatableWidget::NativeOnDeactivated()
{
	APlayerController* OwningPlayerController =
		GetOwningPlayer();

	if (IsValid(OwningPlayerController)
		&& bHasSavedMouseCursorState)
	{
		OwningPlayerController->SetShowMouseCursor(
			bPreviousMouseCursorVisible);
	}

	bHasSavedMouseCursorState = false;

	Super::NativeOnDeactivated();
}
