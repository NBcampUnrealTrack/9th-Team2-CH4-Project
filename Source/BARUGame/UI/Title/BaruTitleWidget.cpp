// BaruTitleWidget.cpp

#include "UI/Title/BaruTitleWidget.h"
#include "Components/Button.h"
#include "Kismet/KismetSystemLibrary.h"

#include "BaruLog.h"

UBaruTitleWidget::UBaruTitleWidget(
	const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
{
	/**
	 * 타이틀 화면에서는 캐릭터를 조작하지 않고
	 * 마우스와 UI만 사용한다.
	 */
	InputConfig = EBaruWidgetInputMode::Menu;
	
	GameMouseCaptureMode =
		EMouseCaptureMode::NoCapture;
}

void UBaruTitleWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	
	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("타이틀 UI가 활성화되었습니다. Widget=%s"),
		*GetName()
		);
}

void UBaruTitleWidget::NativeOnDeactivated()
{
	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("타이틀 UI가 비활성화되었습니다. Widget=%s"),
		*GetName()
		);
	
	Super::NativeOnDeactivated();
}

void UBaruTitleWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	if (IsValid(Button_StartGame))
	{
		Button_StartGame->OnClicked.AddDynamic(this, &UBaruTitleWidget::HandleStartGameClicked);
	}
	
	if (IsValid(Button_QuitGame))
	{
		Button_QuitGame->OnClicked.AddDynamic(this, &ThisClass::HandleQuitGameClicked);
	}
}

void UBaruTitleWidget::HandleStartGameClicked()
{
	BARU_LOG(LogBaruUI, Log, TEXT("게임 시작 버튼이 클릭되었습니다. 로비 이동을 요청합니다"));
	
	BP_OnEnterLobbyRequested();
	
}

void UBaruTitleWidget::HandleQuitGameClicked()
{
	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("게임 종료 버튼이 클릭되었습니다."));
	
	APlayerController* OwningPlayerController =
		GetOwningPlayer();
	
	if (!IsValid(OwningPlayerController))
	{
		BARU_LOG(
		LogBaruUI,
		Warning,
		TEXT("게임을 종료할 PlayerController를 찾지 못했습니다."));
	
		return;
	}
	
	UKismetSystemLibrary::QuitGame(
		this,
		OwningPlayerController,
		EQuitPreference::Quit,
		false);
}