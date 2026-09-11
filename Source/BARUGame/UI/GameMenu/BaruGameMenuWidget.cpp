// BaruGameMenuWidget.cpp

#include "UI/GameMenu/BaruGameMenuWidget.h"

#include "BaruLog.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Subsystems/BaruSessionSubsystem.h"
#include "Settings/BaruGameUserSettings.h"
#include "Settings/BaruPlayerCustomSettings.h"

namespace
{
	const FString FrameRate30 = TEXT("30 FPS");
	const FString FrameRate60 = TEXT("60 FPS");
	const FString FrameRate120 = TEXT("120 FPS");
	const FString FrameRate144 = TEXT("144 FPS");
	
	FString FrameRateTypeToString(EBaruFrameRateLimit FrameRate)
	{
		switch (FrameRate)
		{
		case EBaruFrameRateLimit::FPS_30:
			return FrameRate30;
			
		case EBaruFrameRateLimit::FPS_60:
			return FrameRate60;
			
		case EBaruFrameRateLimit::FPS_120:
			return FrameRate120;
			
		case EBaruFrameRateLimit::FPS_144:
			return FrameRate144;
			
		default:
			return FrameRate60;
		}
	}
	
	EBaruFrameRateLimit StringToFrameRateType(
		const FString& SelectedItem)
	{
		if (SelectedItem == FrameRate30)
		{
			return EBaruFrameRateLimit::FPS_30;
		}
		
		if (SelectedItem == FrameRate120)
		{
			return EBaruFrameRateLimit::FPS_120;
		}
		
		if (SelectedItem == FrameRate144)
		{
			return EBaruFrameRateLimit::FPS_144;
		}
		
		return EBaruFrameRateLimit::FPS_60;
	}
	
	void SetDecimalText(
		UTextBlock* TextBlock,
		float Value,
		int32 FractionalDigits)
	{
		if (!IsValid(TextBlock))
		{
			return;
		}
		
		FNumberFormattingOptions FormatOptions;
		FormatOptions.MinimumFractionalDigits = FractionalDigits;
		FormatOptions.MaximumFractionalDigits = FractionalDigits;
		
		TextBlock->SetText(
			FText::AsNumber(Value, &FormatOptions));
	}
	
	void SetPercentText(
		UTextBlock* TextBlock,
		float Value)
	{
		if (!IsValid(TextBlock))
		{
			return;
		}
		
		const int32 Percent = 
			FMath::RoundToInt(
				FMath::Clamp(Value, 0.0f, 1.0f) * 100.0f);
		
		TextBlock->SetText(
			FText::FromString(
				FString::Printf(TEXT("%d%%"), Percent)));
	}
}

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

	// 기본 메뉴
	Button_Resume->OnClicked.AddUniqueDynamic(
		this,
		&ThisClass::HandleResumeClicked);
	
	Button_OpenOptions->OnClicked.AddUniqueDynamic(
		this,
		&ThisClass::HandleOptionsClicked);
	
	Button_ReturnToLobby->OnClicked.AddUniqueDynamic(
		this,
		&ThisClass::HandleReturnToLobbyClicked);
	
	// 옵션 화면
	Button_BackFromOptions->OnClicked.AddUniqueDynamic(
		this,
		&ThisClass::HandleBackFromOptionsClicked);
	
	Button_Controls->OnClicked.AddUniqueDynamic(
		this,
		&ThisClass::HandleControlsCategoryClicked);
	
	Button_Graphics->OnClicked.AddUniqueDynamic(
		this,
		&ThisClass::HandleGraphicsCategoryClicked);
	
	Button_Audio->OnClicked.AddUniqueDynamic(
		this,
		&ThisClass::HandleAudioCategoryClicked);
	
	Button_Game->OnClicked.AddUniqueDynamic(
		this,
		&ThisClass::HandleGameCategoryClicked);
	
	// 조작
	Slider_MouseSensitivity->OnValueChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleMouseSensitivityChanged);
	
	CheckBox_InvertPitch->OnCheckStateChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleInvertPitchChanged);
	
	CheckBox_InvertYaw->OnCheckStateChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleInvertYawChanged);
	
	CheckBox_MouseSmoothing->OnCheckStateChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleMouseSmoothingChanged);
	
	// 그래픽
	ComboBoxString_FrameRate->OnSelectionChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleFrameRateChanged);
	
	Slider_DisplayGamma->OnValueChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleDisplayGammaChanged);
	
	CheckBox_MotionBlur->OnCheckStateChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleMotionBlurChanged);
	
	CheckBox_VSync->OnCheckStateChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleVSyncChanged);
	
	// 오디오
	Slider_MasterVolume->OnValueChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleMasterVolumeChanged);
	
	Slider_BGMVolume->OnValueChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleBGMVolumeChanged);
	
	Slider_SFXVolume->OnValueChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleSFXVolumeChanged);
	
	Slider_MonsterVolume->OnValueChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleMonsterVolumeChanged);
	
	// 게임
	Slider_FieldOfView->OnValueChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleFieldOfViewChanged);
	
	Slider_CameraShake->OnValueChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleCameraShakeChanged);
	
	Slider_HitScreenEffect->OnValueChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleHitScreenEffectChanged);
	
	CheckBox_ShowCrosshair->OnCheckStateChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleShowCrosshairChanged);
	
	CheckBox_EnableSubtitles->OnCheckStateChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleEnableSubtitlesChanged);
	
	InitializeOptionWidgets();
	
	PlayerCustomSettings =
		UBaruPlayerCustomSettings::LoadOrCreateSettings();
	
	LoadSettingsToWidgets();
}

void UBaruGameMenuWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	
	if (bOpenedFromTitle)
	{
		// 타이틀에서 열었으면 옵션 화면을 표시한다.
		ShowOptionsMenu();
	}
	else
	{
		// 게임 중 ESC로 열었으면 기본 메뉴를 표시한다.
		ShowMainMenu();
	}
}

void UBaruGameMenuWidget::NativeOnDeactivated()
{
	ApplyAndSaveSettings();
	bOpenedFromTitle = false;
	Super::NativeOnDeactivated();
}

bool UBaruGameMenuWidget::NativeOnHandleBackAction()
{
	if (IsValid(WidgetSwitcher_MenuView)
		&& WidgetSwitcher_MenuView->GetActiveWidgetIndex()
		== OptionsViewIndex)
	{
		ApplyAndSaveSettings();
		
		// 타이틀에서 옵션을 열었다면 옵션 위젯 자체를 닫는다.
		// 아래에 있던 타이틀 위젯이 다시 표시된다.
		if (bOpenedFromTitle)
		{
			DeactivateWidget();
			return true;
		}
		
		// 게임 중 ESC 메뉴에서 옵션을 열었다면
		// ESC 기본 메뉴로 돌아간다.
		ShowMainMenu();
		return true;
	}

	// 기본 메뉴에서 ESC를 누르면 메뉴를 닫는다.
	DeactivateWidget();
	return true;
}

void UBaruGameMenuWidget::InitializeOptionWidgets()
{
	bIsUpdatingOptionWidgets = true;
	
	ComboBoxString_FrameRate->ClearOptions();
	ComboBoxString_FrameRate->AddOption(FrameRate30);
	ComboBoxString_FrameRate->AddOption(FrameRate60);
	ComboBoxString_FrameRate->AddOption(FrameRate120);
	ComboBoxString_FrameRate->AddOption(FrameRate144);
	
	bIsUpdatingOptionWidgets = false;
}

void UBaruGameMenuWidget::LoadSettingsToWidgets()
{
	bIsUpdatingOptionWidgets = true;
	
	if (!IsValid(PlayerCustomSettings))
	{
		PlayerCustomSettings =
			UBaruPlayerCustomSettings::LoadOrCreateSettings();
	}
	
	if (IsValid(PlayerCustomSettings))
	{
		Slider_MouseSensitivity->SetValue(
			PlayerCustomSettings->MouseSensitivity);
		
		CheckBox_InvertPitch->SetIsChecked(
			PlayerCustomSettings->bInvertMousePitch);
		
		CheckBox_InvertYaw->SetIsChecked(
			PlayerCustomSettings->bInvertMouseYaw);
		
		CheckBox_MouseSmoothing->SetIsChecked(
			PlayerCustomSettings->bEnableMouseSmoothing);
		
		Slider_MasterVolume->SetValue(
			PlayerCustomSettings->MasterVolume);
		
		Slider_BGMVolume->SetValue(
			PlayerCustomSettings->BGMVolume);
		
		Slider_SFXVolume->SetValue(
			PlayerCustomSettings->SFXVolume);
		
		Slider_MonsterVolume->SetValue(
			PlayerCustomSettings->MonsterVolume);
		
		Slider_FieldOfView->SetValue(
			PlayerCustomSettings->FieldOfView);
		
		Slider_CameraShake->SetValue(
			PlayerCustomSettings->CameraShakeIntensity);

		Slider_HitScreenEffect->SetValue(
			PlayerCustomSettings->HitScreenEffectIntensity);
		
		CheckBox_ShowCrosshair->SetIsChecked(
			PlayerCustomSettings->bShowCrosshair);
		
		CheckBox_EnableSubtitles->SetIsChecked(
			PlayerCustomSettings->bEnableSubtitles);
	}
	
	if (UBaruGameUserSettings* GameSettings =
		UBaruGameUserSettings::GetBaruGameUserSettings())
	{
		ComboBoxString_FrameRate->SetSelectedOption(
			FrameRateTypeToString(
				GameSettings->GetFrameRateLimitType()));
		
		Slider_DisplayGamma->SetValue(
			GameSettings->GetDisplayGamma());
		
		CheckBox_MotionBlur->SetIsChecked(
			GameSettings->IsMotionBlurEnabled());
		
		CheckBox_VSync->SetIsChecked(
			GameSettings->IsVSyncEnabled());
	}
	
	UpdateMouseSensitivityText(
		Slider_MouseSensitivity->GetValue());
	
	UpdateDisplayGammaText(
		Slider_DisplayGamma->GetValue());
	
	UpdateMasterVolumeText(
		Slider_MasterVolume->GetValue());
	
	UpdateBGMVolumeText(
		Slider_BGMVolume->GetValue());
	
	UpdateSFXVolumeText(
		Slider_SFXVolume->GetValue());
	
	UpdateMonsterVolumeText(
		Slider_MonsterVolume->GetValue());
	
	UpdateFieldOfViewText(
		Slider_FieldOfView->GetValue());
	
	UpdateCameraShakeText(
		Slider_CameraShake->GetValue());
	
	UpdateHitScreenEffectText(
		 Slider_HitScreenEffect->GetValue());
	
	bIsUpdatingOptionWidgets = false;
	bHasPendingSettingsChanges = false;
}

void UBaruGameMenuWidget::ApplyAndSaveSettings()
{
	if (!bHasPendingSettingsChanges)
	{
		return;
	}
	
	if (UBaruGameUserSettings* GameSettings =
		UBaruGameUserSettings::GetBaruGameUserSettings())
	{
		GameSettings->ApplyNonResolutionSettings();
		GameSettings->SaveSettings();
	}
	
	if (IsValid(PlayerCustomSettings))
	{
		PlayerCustomSettings->SaveCustomSettings();
	}
	
	bHasPendingSettingsChanges = false;
	
	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("옵션 설정을 적용하고 저장했습니다."));
}

void UBaruGameMenuWidget::OpenOptionsFromTitle()
{
	// 이 함수는 UI Layer에 위젯이 Push된 다음 호출된다.
	bOpenedFromTitle = true;
	
	ShowOptionsMenu();
	
	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("타이틀 화면에서 옵션 화면을 열었습니다."));
}

void UBaruGameMenuWidget::ShowMainMenu()
{
	WidgetSwitcher_MenuView->SetActiveWidgetIndex(
		MainMenuViewIndex);
}

void UBaruGameMenuWidget::ShowOptionsMenu()
{
	WidgetSwitcher_MenuView->SetActiveWidgetIndex(
		OptionsViewIndex);

	// 옵션 화면을 열면 첫 번째인 조작 탭을 표시한다.
	ShowOptionsCategory(ControlsCategoryIndex);
}

void UBaruGameMenuWidget::ShowOptionsCategory(
	int32 CategoryIndex)
{
	const int32 CategoryCount =
		WidgetSwitcher_OptionsCategory->GetChildrenCount();
	
	if (CategoryCount <= 0)
	{
		return;
	}
	
	WidgetSwitcher_OptionsCategory->SetActiveWidgetIndex(
		FMath::Clamp(CategoryIndex, 0, CategoryCount - 1));
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
	ApplyAndSaveSettings();
	
	if (bOpenedFromTitle)
	{
		DeactivateWidget();
		return;
	}
	
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

void UBaruGameMenuWidget::HandleMouseSensitivityChanged(
	float Value)
{
	if (bIsUpdatingOptionWidgets)
	{
		return;
	}

	Value = FMath::Clamp(Value, 0.01f, 2.0f);
	UpdateMouseSensitivityText(Value);

	if (IsValid(PlayerCustomSettings))
	{
		PlayerCustomSettings->MouseSensitivity = Value;
		bHasPendingSettingsChanges = true;
	}
}

void UBaruGameMenuWidget::HandleInvertPitchChanged(
	bool bIsChecked)
{
	if (!bIsUpdatingOptionWidgets
		&& IsValid(PlayerCustomSettings))
	{
		PlayerCustomSettings->bInvertMousePitch = bIsChecked;
		bHasPendingSettingsChanges = true;
	}
}

void UBaruGameMenuWidget::HandleInvertYawChanged(
	bool bIsChecked)
{
	if (!bIsUpdatingOptionWidgets
		&& IsValid(PlayerCustomSettings))
	{
		PlayerCustomSettings->bInvertMouseYaw = bIsChecked;
		bHasPendingSettingsChanges = true;
	}
}

void UBaruGameMenuWidget::HandleMouseSmoothingChanged(
	bool bIsChecked)
{
	if (!bIsUpdatingOptionWidgets
		&& IsValid(PlayerCustomSettings))
	{
		PlayerCustomSettings->bEnableMouseSmoothing = bIsChecked;
		bHasPendingSettingsChanges = true;
	}
}

void UBaruGameMenuWidget::HandleFrameRateChanged(
	FString SelectedItem,
	ESelectInfo::Type SelectionType)
{
	if (bIsUpdatingOptionWidgets)
	{
		return;
	}

	if (UBaruGameUserSettings* GameSettings =
		UBaruGameUserSettings::GetBaruGameUserSettings())
	{
		GameSettings->SetFrameRateLimitType(
			StringToFrameRateType(SelectedItem));

		bHasPendingSettingsChanges = true;
	}
}

void UBaruGameMenuWidget::HandleDisplayGammaChanged(
	float Value)
{
	if (bIsUpdatingOptionWidgets)
	{
		return;
	}

	Value = FMath::Clamp(Value, 1.5f, 3.0f);
	UpdateDisplayGammaText(Value);

	if (UBaruGameUserSettings* GameSettings =
		UBaruGameUserSettings::GetBaruGameUserSettings())
	{
		GameSettings->SetDisplayGamma(Value);
		bHasPendingSettingsChanges = true;
	}
}

void UBaruGameMenuWidget::HandleMotionBlurChanged(
	bool bIsChecked)
{
	if (bIsUpdatingOptionWidgets)
	{
		return;
	}

	if (UBaruGameUserSettings* GameSettings =
		UBaruGameUserSettings::GetBaruGameUserSettings())
	{
		GameSettings->SetMotionBlurEnabled(bIsChecked);
		bHasPendingSettingsChanges = true;
	}
}

void UBaruGameMenuWidget::HandleVSyncChanged(
	bool bIsChecked)
{
	if (bIsUpdatingOptionWidgets)
	{
		return;
	}

	if (UBaruGameUserSettings* GameSettings =
		UBaruGameUserSettings::GetBaruGameUserSettings())
	{
		GameSettings->SetVSyncEnabled(bIsChecked);
		bHasPendingSettingsChanges = true;
	}
}

void UBaruGameMenuWidget::HandleMasterVolumeChanged(
	float Value)
{
	if (bIsUpdatingOptionWidgets)
	{
		return;
	}

	Value = FMath::Clamp(Value, 0.0f, 1.0f);
	UpdateMasterVolumeText(Value);

	if (IsValid(PlayerCustomSettings))
	{
		PlayerCustomSettings->MasterVolume = Value;
		PlayerCustomSettings->OnAudioVolumeChanged.Broadcast(
			TEXT("Master"),
			Value);

		bHasPendingSettingsChanges = true;
	}
}

void UBaruGameMenuWidget::HandleBGMVolumeChanged(
	float Value)
{
	if (bIsUpdatingOptionWidgets)
	{
		return;
	}

	Value = FMath::Clamp(Value, 0.0f, 1.0f);
	UpdateBGMVolumeText(Value);

	if (IsValid(PlayerCustomSettings))
	{
		PlayerCustomSettings->BGMVolume = Value;
		PlayerCustomSettings->OnAudioVolumeChanged.Broadcast(
			TEXT("BGM"),
			Value);

		bHasPendingSettingsChanges = true;
	}
}

void UBaruGameMenuWidget::HandleSFXVolumeChanged(
	float Value)
{
	if (bIsUpdatingOptionWidgets)
	{
		return;
	}

	Value = FMath::Clamp(Value, 0.0f, 1.0f);
	UpdateSFXVolumeText(Value);

	if (IsValid(PlayerCustomSettings))
	{
		PlayerCustomSettings->SFXVolume = Value;
		PlayerCustomSettings->OnAudioVolumeChanged.Broadcast(
			TEXT("SFX"),
			Value);

		bHasPendingSettingsChanges = true;
	}
}

void UBaruGameMenuWidget::HandleMonsterVolumeChanged(
	float Value)
{
	if (bIsUpdatingOptionWidgets)
	{
		return;
	}

	Value = FMath::Clamp(Value, 0.0f, 1.0f);
	UpdateMonsterVolumeText(Value);

	if (IsValid(PlayerCustomSettings))
	{
		PlayerCustomSettings->MonsterVolume = Value;
		PlayerCustomSettings->OnAudioVolumeChanged.Broadcast(
			TEXT("Monster"),
			Value);

		bHasPendingSettingsChanges = true;
	}
}

void UBaruGameMenuWidget::HandleFieldOfViewChanged(
	float Value)
{
	if (bIsUpdatingOptionWidgets)
	{
		return;
	}

	Value = FMath::Clamp(Value, 70.0f, 110.0f);
	UpdateFieldOfViewText(Value);

	if (IsValid(PlayerCustomSettings))
	{
		PlayerCustomSettings->FieldOfView = Value;
		PlayerCustomSettings->OnFOVChanged.Broadcast(Value);

		bHasPendingSettingsChanges = true;
	}
}

void UBaruGameMenuWidget::HandleCameraShakeChanged(
	float Value)
{
	if (bIsUpdatingOptionWidgets)
	{
		return;
	}

	Value = FMath::Clamp(Value, 0.0f, 1.0f);
	UpdateCameraShakeText(Value);

	if (IsValid(PlayerCustomSettings))
	{
		PlayerCustomSettings->CameraShakeIntensity = Value;
		bHasPendingSettingsChanges = true;
	}
}

void UBaruGameMenuWidget::HandleHitScreenEffectChanged(
	float Value)
{
	if (bIsUpdatingOptionWidgets)
	{
		return;
	}

	Value = FMath::Clamp(Value, 0.0f, 1.0f);
	UpdateHitScreenEffectText(Value);

	if (IsValid(PlayerCustomSettings))
	{
		PlayerCustomSettings->HitScreenEffectIntensity = Value;
		bHasPendingSettingsChanges = true;
	}
}

void UBaruGameMenuWidget::HandleShowCrosshairChanged(
	bool bIsChecked)
{
	if (!bIsUpdatingOptionWidgets
		&& IsValid(PlayerCustomSettings))
	{
		PlayerCustomSettings->bShowCrosshair = bIsChecked;
		bHasPendingSettingsChanges = true;
	}
}

void UBaruGameMenuWidget::HandleEnableSubtitlesChanged(
	bool bIsChecked)
{
	if (!bIsUpdatingOptionWidgets
		&& IsValid(PlayerCustomSettings))
	{
		PlayerCustomSettings->bEnableSubtitles = bIsChecked;
		bHasPendingSettingsChanges = true;
	}
}

void UBaruGameMenuWidget::UpdateMouseSensitivityText(
	float Value)
{
	SetDecimalText(
		Text_MouseSensitivityValue,
		Value,
		2);
}

void UBaruGameMenuWidget::UpdateDisplayGammaText(
	float Value)
{
	SetDecimalText(
		Text_DisplayGammaValue,
		Value,
		1);
}

void UBaruGameMenuWidget::UpdateMasterVolumeText(
	float Value)
{
	SetPercentText(Text_MasterVolumeValue, Value);
}

void UBaruGameMenuWidget::UpdateBGMVolumeText(
	float Value)
{
	SetPercentText(Text_BGMVolumeValue, Value);
}

void UBaruGameMenuWidget::UpdateSFXVolumeText(
	float Value)
{
	SetPercentText(Text_SFXVolumeValue, Value);
}

void UBaruGameMenuWidget::UpdateMonsterVolumeText(
	float Value)
{
	SetPercentText(Text_MonsterVolumeValue, Value);
}

void UBaruGameMenuWidget::UpdateFieldOfViewText(
	float Value)
{
	SetDecimalText(
		Text_FieldOfViewValue,
		Value,
		0);
}

void UBaruGameMenuWidget::UpdateCameraShakeText(
	float Value)
{
	SetPercentText(Text_CameraShakeValue, Value);
}

void UBaruGameMenuWidget::UpdateHitScreenEffectText(
	float Value)
{
	SetPercentText(Text_HitScreenEffectValue, Value);
}

void UBaruGameMenuWidget::HandleReturnToLobbyClicked()
{
	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("ESC 메뉴에서 로비로 돌아가기를 요청했습니다."));

	APlayerController* OwningPlayerController =
		GetOwningPlayer();

	if (!IsValid(OwningPlayerController))
	{
		BARU_LOG(
			LogBaruUI,
			Warning,
			TEXT("로비로 돌아갈 PlayerController를 찾지 못했습니다."));
		return;
	}
	
	UWorld* World = GetWorld();
	
	if (!IsValid(World))
	{
		BARU_LOG(
			LogBaruUI,
			Warning,
			TEXT("로비로 돌아갈 World를 찾지 못했습니다."));
		
		return;
	}
	
	/**
	 *	Listen Server의 로컬 플레이어는 서버 자신이다.
	 *	방장이 로비 이동을 요청하면 세션을 유지하면서
	 *	현재 접속한 모든 참가자를 로비로 이동시킨다.
	 */
	if (World->GetNetMode() == NM_ListenServer)
	{
		BARU_LOG(
			LogBaruUI,
			Log,
			TEXT("Listen Server 방장이 모든 참가자를 로비로 이동시킵니다."));
		
		DeactivateWidget();
		
		World->ServerTravel(
			TEXT("/Game/BARUGame/Maps/MainLobbyLevel?listen"),
			true);
		
		return;
	}
	
	/**
	 *	일반 클라이언트라면 현재 온라인 세션에서 빠진다.
	 *	false이므로 SessionSubsystem이 MainMenuLevel로 이동시키지는 않는다.
	 */
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UBaruSessionSubsystem* SessionSubsystem =
			GameInstance->GetSubsystem<UBaruSessionSubsystem>())
		{
			SessionSubsystem->DestroySession(false);
		}
	}
	
	// 열려있는 ESC 메뉴를 닫고 입력 상태를 정리한다.
	DeactivateWidget();
	
	/*
	 *	ServerTravel이 아니라 이 로컬 PlayerController의 ClientTravel을
	 *	호출하므로 해당 클라이언트만 MainLobbyLevel로 이동한다.
	 */
	OwningPlayerController->ClientTravel(
		TEXT("/Game/BARUGame/Maps/MainLobbyLevel"),
		ETravelType::TRAVEL_Absolute);
}
