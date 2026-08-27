// BaruMainHUDWidget.cpp

#include "UI/HUD/BaruMainHUDWidget.h"

#include "BaruLog.h"

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