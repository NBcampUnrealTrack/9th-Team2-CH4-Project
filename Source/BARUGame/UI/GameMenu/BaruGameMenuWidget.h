// BaruGameMenuWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Types/SlateEnums.h"
#include "UI/Foundation/BaruActivatableWidget.h"

#include "BaruGameMenuWidget.generated.h"

class UBaruPlayerCustomSettings;
class UButton;
class UCheckBox;
class UComboBoxString;
class USlider;
class UTextBlock;
class UWidgetSwitcher;

/**
 * ESC를 눌렀을 때 로컬 플레이어에게만 표시되는 게임 메뉴
 *
 * 게임 전체를 일시정지하지 않고
 * 계속하기, 옵션, 로비로 돌아가기 기능을 제공한다.
 */
UCLASS(Abstract, Blueprintable)
class BARUGAME_API UBaruGameMenuWidget : public UBaruActivatableWidget
{
	GENERATED_BODY()

public:
	UBaruGameMenuWidget(
		const FObjectInitializer& ObjectInitializer);
	
	// 타이틀 화면에서 옵션 화면만 바로 연다.
	UFUNCTION(BlueprintCallable, Category = "BARU|UI|GameMenu")
	void OpenOptionsFromTitle();

protected:
	virtual void NativeOnInitialized() override;

	virtual void NativeOnActivated() override;
	
	virtual void NativeOnDeactivated() override;

	//CommonUI의 ESC 또는 Back 입력을 처리한다.
	virtual bool NativeOnHandleBackAction() override;

	// 기본 ESC 메뉴 화면을 표시한다.
	void ShowMainMenu();

	// 옵션 화면을 표시한다.
	void ShowOptionsMenu();

	// 옵션 화면의 지정된 카테고리를 표시한다.
	void ShowOptionsCategory(int32 CategoryIndex);
	
	// 옵션 초기화, 불러오기, 저장
	void InitializeOptionWidgets();
	void LoadSettingsToWidgets();
	void ApplyAndSaveSettings();
	
	// 숫자 표시
	void UpdateMouseSensitivityText(float Value);
	void UpdateDisplayGammaText(float Value);
	void UpdateMasterVolumeText(float Value);
	void UpdateBGMVolumeText(float Value);
	void UpdateSFXVolumeText(float Value);
	void UpdateMonsterVolumeText(float Value);
	void UpdateFieldOfViewText(float Value);
	void UpdateCameraShakeText(float Value);
	void UpdateHitScreenEffectText(float Value);
	
	// 계속하기
	UFUNCTION()
	void HandleResumeClicked();

	// 옵션 열기
	UFUNCTION()
	void HandleOptionsClicked();
	
	// 로비로 돌아가기
	UFUNCTION()
	void HandleReturnToLobbyClicked();

	// 옵션에서 뒤로가기
	UFUNCTION()
	void HandleBackFromOptionsClicked();

	// 조작 탭
	UFUNCTION()
	void HandleControlsCategoryClicked();

	// 그래픽 탭
	UFUNCTION()
	void HandleGraphicsCategoryClicked();

	// 오디오 탭
	UFUNCTION()
	void HandleAudioCategoryClicked();

	// 게임 탭
	UFUNCTION()
	void HandleGameCategoryClicked();
	
	// 조작 설정
	UFUNCTION()
	void HandleMouseSensitivityChanged(float Value);
	
	UFUNCTION()
	void HandleInvertPitchChanged(bool bIsChecked);
	
	UFUNCTION()
	void HandleInvertYawChanged(bool bIsChecked);
	
	UFUNCTION()
	void HandleMouseSmoothingChanged(bool bIsChecked);
	
	// 그래픽 설정
	UFUNCTION()
	void HandleFrameRateChanged(
		FString SelectedItem,
		ESelectInfo::Type SelectionType);
	
	UFUNCTION()
	void HandleDisplayGammaChanged(float Value);
	
	UFUNCTION()
	void HandleMotionBlurChanged(bool bIsChecked);
	
	UFUNCTION()
	void HandleVSyncChanged(bool bIsChecked);
	
	// 오디오 설정
	UFUNCTION()
	void HandleMasterVolumeChanged(float Value);
	
	UFUNCTION()
	void HandleBGMVolumeChanged(float Value);
	
	UFUNCTION()
	void HandleSFXVolumeChanged(float Value);
	
	UFUNCTION()
	void HandleMonsterVolumeChanged(float Value);
	
	// 게임 설정
	UFUNCTION()
	void HandleFieldOfViewChanged(float Value);
	
	UFUNCTION()
	void HandleCameraShakeChanged(float Value);
	
	UFUNCTION()
	void HandleHitScreenEffectChanged(float Value);
	
	UFUNCTION()
	void HandleShowCrosshairChanged(bool bIsChecked);
	
	UFUNCTION()
	void HandleEnableSubtitlesChanged(bool bIsChecked);

protected:
	// 0: ESC 기본 메뉴, 1: 옵션 화면
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcher_MenuView;

	// 0: 조작, 1: 그래픽, 2: 오디오, 3: 게임
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcher_OptionsCategory;

	// 기본 메뉴 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Resume;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_OpenOptions;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_ReturnToLobby;

	// 옵션 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_BackFromOptions;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Controls;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Graphics;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Audio;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Game;
	
	// Controls
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_MouseSensitivity;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_MouseSensitivityValue;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> CheckBox_InvertPitch;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> CheckBox_InvertYaw;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> CheckBox_MouseSmoothing;
	
	// Graphics
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> ComboBoxString_FrameRate;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_DisplayGamma;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_DisplayGammaValue;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> CheckBox_MotionBlur;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> CheckBox_VSync;
	
	// Audio
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_MasterVolume;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_MasterVolumeValue;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_BGMVolume;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_BGMVolumeValue;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_SFXVolume;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SFXVolumeValue;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_MonsterVolume;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_MonsterVolumeValue;
	
	// GamePlay
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_FieldOfView;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_FieldOfViewValue;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_CameraShake;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CameraShakeValue;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_HitScreenEffect;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_HitScreenEffectValue;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> CheckBox_ShowCrosshair;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> CheckBox_EnableSubtitles;
	
	// 불러온 사용자 설정 객체를 GC로부터 보호한다.
	UPROPERTY(Transient)
	TObjectPtr<UBaruPlayerCustomSettings> PlayerCustomSettings;
	

private:
	bool bIsUpdatingOptionWidgets = false;
	bool bHasPendingSettingsChanges = false;
	
	// true이면 옵션 뒤로가기 시 ESC 메뉴가 아니라 타이틀로 복귀한다.
	bool bOpenedFromTitle = false;
	
	static constexpr int32 MainMenuViewIndex = 0;
	static constexpr int32 OptionsViewIndex = 1;

	static constexpr int32 ControlsCategoryIndex = 0;
	static constexpr int32 GraphicsCategoryIndex = 1;
	static constexpr int32 AudioCategoryIndex = 2;
	static constexpr int32 GameCategoryIndex = 3;
};
