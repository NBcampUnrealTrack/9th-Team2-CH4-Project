// BaruMainHUDWidget.cpp

#include "UI/HUD/BaruMainHUDWidget.h"

#include "BaruLog.h"

UBaruMainHUDWidget::UBaruMainHUDWidget(
	const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
{
	// Main HUD는 플레이 중 항상 표시되지만
	// 캐릭터와 카메라 입력을 막으면 안 된다.
	InputConfig = EBaruWidgetInputMode::Game;

	GameMouseCaptureMode =
		EMouseCaptureMode::CapturePermanently;
}

void UBaruMainHUDWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("Main HUD가 활성화되었습니다. Widget=%s"),
		*GetName());
}

void UBaruMainHUDWidget::NativeOnDeactivated()
{
	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("Main HUD가 비활성화되었습니다. Widget=%s"),
		*GetName());

	Super::NativeOnDeactivated();
}
