// BaruActivatableWidget.cpp

#include "UI/Foundation/BaruActivatableWidget.h"

#include "CommonInputModeTypes.h"
#include "Input/UIActionBindingHandle.h"

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
