// BaruTitleWidget.cpp

#include "UI/Title/BaruTitleWidget.h"

#include "BaruLog.h"
#include "Components/Button.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Subsystems/BaruSaveGameSubsystem.h"
#include "UI/BaruUITags.h"
#include "UI/GameMenu/BaruGameMenuWidget.h"
#include "UI/Subsystem/BaruUIManagerSubsystem.h"
#include "UI/Title/BaruCreateIDWidget.h"
#include "Engine/GameInstance.h"


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
	
	if (IsValid(Button_Options))
	{
		Button_Options->OnClicked.AddUniqueDynamic(
			this,
			&ThisClass::HandleOptionsClicked);
	}
	
	if (IsValid(Button_QuitGame))
	{
		Button_QuitGame->OnClicked.AddDynamic(this, &ThisClass::HandleQuitGameClicked);
	}
}

void UBaruTitleWidget::HandleStartGameClicked()
{
	// [09.13] 닉네임 생성 및 프로필 검사 분기
	UGameInstance* GI = GetGameInstance();
	UBaruSaveGameSubsystem* SaveSubsystem = GI ? GI->GetSubsystem<UBaruSaveGameSubsystem>() : nullptr;

	if (!SaveSubsystem)
	{
		BP_OnEnterLobbyRequested();
		return;
	}

	// 1. 세이브 파일이 이미 존재하는 경우 -> 로비 즉시 진입
	if (SaveSubsystem->DoesProfileExist())
	{
		BARU_LOG(LogBaruUI, Log, TEXT("프로필 확인 완료 (%s). 로비로 이동합니다."), *SaveSubsystem->GetActiveProfilePlayerName());
		BP_OnEnterLobbyRequested();
		return;
	}

	// 2. 세이브 파일이 없는 경우 -> 아이디 생성 모달 팝업 띄우기
	if (!CreateIDWidgetClass)
	{
		BARU_LOG(LogBaruUI, Error, TEXT("CreateIDWidgetClass가 할당되지 않았습니다."));
		return;
	}

	ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	UBaruUIManagerSubsystem* UIManager = LocalPlayer ? LocalPlayer->GetSubsystem<UBaruUIManagerSubsystem>() : nullptr;

	if (UIManager)
	{
		UCommonActivatableWidget* Pushed = UIManager->PushWidgetToLayer(BaruUITags::UI_Layer_Modal.GetTag(), CreateIDWidgetClass);
		if (UBaruCreateIDWidget* IDWidget = Cast<UBaruCreateIDWidget>(Pushed))
		{
			IDWidget->OnIDCreationSuccess.AddUniqueDynamic(this, &UBaruTitleWidget::HandleIDCreationCompleted);
		}
	}
}

void UBaruTitleWidget::HandleOptionsClicked()
{
	if (!OptionsWidgetClass)
	{
		BARU_LOG(
			LogBaruUI,
			Warning,
			TEXT("타이틀에 OptionsWidgetClass가 설정되지 않았습니다."));
		
		return;
	}
	
	ULocalPlayer* LocalPlayer =
		GetOwningLocalPlayer();
	
	if (!IsValid(LocalPlayer))
	{
		BARU_LOG(
			LogBaruUI,
			Warning,
			TEXT("옵션을 열 LocalPlayer를 찾지 못했습니다."));
		
		return;
	}
	
	UBaruUIManagerSubsystem* UIManager =
		LocalPlayer->GetSubsystem<UBaruUIManagerSubsystem>();
	
	if (!IsValid(UIManager))
	{
		BARU_LOG(
			LogBaruUI,
			Warning,
			TEXT("옵션을 열 UIManager를 찾지 못했습니다."));
		
		return;
	}
	
	UCommonActivatableWidget* AddedWidget =
		UIManager->PushWidgetToLayer(
			BaruUITags::UI_Layer_Menu.GetTag(),
			OptionsWidgetClass);
	
	UBaruGameMenuWidget* OptionsWidget =
		Cast<UBaruGameMenuWidget>(AddedWidget);
	
	if (!IsValid(OptionsWidget))
	{
		BARU_LOG(
			LogBaruUI,
			Error,
			TEXT("타이틀 옵션 위젯 생성에 실패했습니다."));
		
		return;
	}
	
	OptionsWidget->OpenOptionsFromTitle();
	
	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("타이틀의 옵션 버튼으로 옵션 UI를 열었습니다."));
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

// [09.13] 닉네임 생성 완료 후 로비 전이 콜백
void UBaruTitleWidget::HandleIDCreationCompleted(const FString& NewID)
{
	BARU_LOG(LogBaruUI, Log, TEXT("새 아이디 등록 완료: %s -> 로비로 이동합니다."), *NewID);
	BP_OnEnterLobbyRequested();
}