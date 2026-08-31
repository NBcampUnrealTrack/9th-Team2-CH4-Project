// BaruLobbyWidget.cpp

#include "UI/Lobby/BaruLobbyWidget.h"

#include "BaruLog.h"

UBaruLobbyWidget::UBaruLobbyWidget(
	const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	/**
	 * 로비 화면도 캐릭터 조작 없이 
	 * 마우스와 UI만 사용한다.
	 */
	InputConfig = EBaruWidgetInputMode::Menu;
	
	GameMouseCaptureMode = EMouseCaptureMode::NoCapture;
}

void UBaruLobbyWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	
	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("로비 UI가 활성화되었습니다. Widget=%s"),
		*GetName()
		);
}

void UBaruLobbyWidget::NativeOnDeactivated()
{
	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("로비 UI가 비활성화 되었습니다. Widget=%s"),
		*GetName()
		);
	
	Super::NativeOnDeactivated();
}