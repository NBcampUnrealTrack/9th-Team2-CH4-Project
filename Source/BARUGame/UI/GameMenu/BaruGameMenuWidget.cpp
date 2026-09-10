// BaruGameMenuWidget.cpp

#include "UI/GameMenu/BaruGameMenuWidget.h"

#include "BaruLog.h"
#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"

UBaruGameMenuWidget::UBaruGameMenuWidget(
	const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
{
	// 로컬 UI만 입력을 받으며 게임 전체를 일시 정지하지 않는다.
	InputConfig = EBaruWidgetInputMode::Menu;

	GameMouseCaptureMode =
		EMouseCaptureMode::NoCapture;

	// CommonUI가 ESC 또는 Back 입력을 이 위젯에 전달한다.
	bIsBackHandler = true;
}

void UBaruGameMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (IsValid(Button_Resume))
	{
		Button_Resume->OnClicked.AddUniqueDynamic(
			this,
			&ThisClass::HandleResumeClicked);
	}

	if (IsValid(Button_OpenOptions))
	{
		Button_OpenOptions->OnClicked.AddUniqueDynamic(
			this,
			&ThisClass::HandleOptionsClicked);
	}

	if (IsValid(Button_QuitGame))
	{
		Button_QuitGame->OnClicked.AddUniqueDynamic(
			this,
			&ThisClass::HandleQuitGameClicked);
	}

	if (IsValid(Button_BackFromOptions))
	{
		Button_BackFromOptions->OnClicked.AddUniqueDynamic(
			this,
			&ThisClass::HandleBackFromOptionsClicked);
	}

	if (IsValid(Button_Controls))
	{
		Button_Controls->OnClicked.AddUniqueDynamic(
			this,
			&ThisClass::HandleControlsCategoryClicked);
	}

	if (IsValid(Button_Graphics))
	{
		Button_Graphics->OnClicked.AddUniqueDynamic(
			this,
			&ThisClass::HandleGraphicsCategoryClicked);
	}

	if (IsValid(Button_Audio))
	{
		Button_Audio->OnClicked.AddUniqueDynamic(
			this,
			&ThisClass::HandleAudioCategoryClicked);
	}

	if (IsValid(Button_Game))
	{
		Button_Game->OnClicked.AddUniqueDynamic(
			this,
			&ThisClass::HandleGameCategoryClicked);
	}
}

void UBaruGameMenuWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	// 메뉴를 다시 열 때 항상 기본 ESC 화면부터 보여준다.
	ShowMainMenu();
}

bool UBaruGameMenuWidget::NativeOnHandleBackAction()
{
	if (IsValid(WidgetSwitcher_MenuView)
		&& WidgetSwitcher_MenuView->GetActiveWidgetIndex()
		== OptionsViewIndex)
	{
		// 옵션 화면에서 ESC를 기본 메뉴로 돌아간다.
		ShowMainMenu();
		return true;
	}

	// 기본 메뉴에서 ESC를 누르면 메뉴를 닫는다.
	DeactivateWidget();
	return true;
}

void UBaruGameMenuWidget::ShowMainMenu()
{
	if (!IsValid(WidgetSwitcher_MenuView))
	{
		BARU_LOG(
			LogBaruUI,
			Warning,
			TEXT("WidgetSwitcher_MenuView가 연결되지 않았습니다."));

		return;
	}

	WidgetSwitcher_MenuView->SetActiveWidgetIndex(
		MainMenuViewIndex);
}

void UBaruGameMenuWidget::ShowOptionsMenu()
{
	if (!IsValid(WidgetSwitcher_MenuView))
	{
		BARU_LOG(
			LogBaruUI,
			Warning,
			TEXT("WidgetSwitcher_MenuView가 연결되지 않았습니다."));

		return;
	}

	WidgetSwitcher_MenuView->SetActiveWidgetIndex(
		OptionsViewIndex);

	// 옵션 화면을 열면 첫 번째인 조작 탭을 표시한다.
	ShowOptionsCategory(ControlsCategoryIndex);
}

void UBaruGameMenuWidget::ShowOptionsCategory(
	int32 CategoryIndex)
{
	if (!IsValid(WidgetSwitcher_OptionsCategory))
	{
		BARU_LOG(
			LogBaruUI,
			Warning,
			TEXT("WidgetSwitcher_OptionsCategory가 연결되지 않았습니다."));

		return;
	}

	const int32 CategoryCount =
		WidgetSwitcher_OptionsCategory->GetChildrenCount();

	if (CategoryCount <= 0)
	{
		BARU_LOG(
			LogBaruUI,
			Warning,
			TEXT("옵션 카테고리 화면이 비어 있습니다."));

		return;
	}

	const int32 SafeCategoryIndex = FMath::Clamp(
		CategoryIndex,
		0,
		CategoryCount - 1);

	WidgetSwitcher_OptionsCategory->SetActiveWidgetIndex(
		SafeCategoryIndex);
}

void UBaruGameMenuWidget::HandleResumeClicked()
{
	DeactivateWidget();
}

void UBaruGameMenuWidget::HandleOptionsClicked()
{
	ShowOptionsMenu();
}

void UBaruGameMenuWidget::HandleBackFromOptionsClicked()
{
	ShowMainMenu();
}

void UBaruGameMenuWidget::HandleControlsCategoryClicked()
{
	ShowOptionsCategory(ControlsCategoryIndex);
}

void UBaruGameMenuWidget::HandleGraphicsCategoryClicked()
{
	ShowOptionsCategory(GraphicsCategoryIndex);
}

void UBaruGameMenuWidget::HandleAudioCategoryClicked()
{
	ShowOptionsCategory(AudioCategoryIndex);
}

void UBaruGameMenuWidget::HandleGameCategoryClicked()
{
	ShowOptionsCategory(GameCategoryIndex);
}

void UBaruGameMenuWidget::HandleQuitGameClicked()
{
	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("ESC 메뉴에서 게임 종료를 요청했습니다."));

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
