// BaruGameMenuWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/BaruActivatableWidget.h"

#include "BaruGameMenuWidget.generated.h"

class UButton;
class UWidgetSwitcher;

/**
 * ESC를 눌렀을 때 로컬 플레이어에게만 표시되는 게임 메뉴
 *
 * 게임 전체를 일시정지하지 않고
 * 계속하기, 옵션, 게임 종료 기능을 제공한다.
 */
UCLASS(Abstract, Blueprintable)
class BARUGAME_API UBaruGameMenuWidget : public UBaruActivatableWidget
{
	GENERATED_BODY()

public:
	UBaruGameMenuWidget(
		const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnInitialized() override;

	virtual void NativeOnActivated() override;

	//CommonUI의 ESC 또는 Back 입력을 처리한다.
	virtual bool NativeOnHandleBackAction() override;

	// 기본 ESC 메뉴 화면을 표시한다.
	void ShowMainMenu();

	// 옵션 화면을 표시한다.
	void ShowOptionsMenu();

	// 옵션 화면의 지정된 카테고리를 표시한다.
	void ShowOptionsCategory(int32 CategoryIndex);

	// 계속하기
	UFUNCTION()
	void HandleResumeClicked();

	// 옵션 열기
	UFUNCTION()
	void HandleOptionsClicked();

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

	// 게임 종료
	UFUNCTION()
	void HandleQuitGameClicked();

protected:
	// 0: ESC 기본 메뉴, 1: 옵션 화면
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcher_MenuView;

	// 0: 조작, 1: 그래픽, 2: 오디오, 3: 게임
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> WidgetSwitcher_OptionsCategory;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Resume;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_OpenOptions;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_QuitGame;

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

private:
	static constexpr int32 MainMenuViewIndex = 0;
	static constexpr int32 OptionsViewIndex = 1;

	static constexpr int32 ControlsCategoryIndex = 0;
	static constexpr int32 GraphicsCategoryIndex = 1;
	static constexpr int32 AudioCategoryIndex = 2;
	static constexpr int32 GameCategoryIndex = 3;
};
