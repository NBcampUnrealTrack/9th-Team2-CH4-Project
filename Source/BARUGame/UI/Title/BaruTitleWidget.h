// BaruTitleWidge.h

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/BaruActivatableWidget.h"

#include "BaruTitleWidget.generated.h"

class UBaruGameMenuWidget;
class UButton;
class UBaruCreateIDWidget;

/**
 * 게임 실행 후 처음 표시되는 타이틀 화면의 C++ 기반 클래스
 * 
 * 게임 시작, 옵션, 크래딧, 게임 종료 메뉴를 제공한다.
 */
UCLASS(Abstract, Blueprintable)
class BARUGAME_API UBaruTitleWidget
	: public UBaruActivatableWidget
{
	GENERATED_BODY()
	
public:
	UBaruTitleWidget(
		const FObjectInitializer& ObjectInitializer);
	
protected:
	virtual void NativeOnActivated() override;
	
	virtual void NativeOnDeactivated() override;
	
	virtual void NativeOnInitialized() override;

	/**
	 * 게임 시작 버튼이 클릭되었을 때 호출된다.
	 */
	UFUNCTION()
	void HandleStartGameClicked();
	
	/**
	 *	옵션 버튼이 클릭되었을 때 호출된다.
	 */
	UFUNCTION()
	void HandleOptionsClicked();
	
	/**
	 *	게임 종료 버튼이 클릭되었을 때 호출된다.
	 */
	UFUNCTION()
	void HandleQuitGameClicked();

	/**
	 * 타이틀에서 로비로 이동해 달라고 요청한다.
	 * 실제 Level 이동은 Level/GameMode 담당 시스템에서 처린한다.
	 */
	UFUNCTION(
		BlueprintImplementableEvent, Category = "BARU|UI|Title")
	void BP_OnEnterLobbyRequested();
	
	// [09.13] 닉네임 생성 핸들러
	UFUNCTION()
	void HandleIDCreationCompleted(const FString& NewID);
	
	
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_StartGame;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Options;
	
	// 게임 종료 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_QuitGame;
	
	/**
	 *	타이틀의 옵션 버튼으로 열 위젯 클래스.
	 *	WBP_Title의 Class Defaults에서 WBP_GameMenu를 지정한다.
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "BARU|UI|Title")
	TSubclassOf<UBaruGameMenuWidget> OptionsWidgetClass;
	
	// [09.13] Modale로 띄울 아이디 생성 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|UI|Title")
	TSubclassOf<UBaruCreateIDWidget> CreateIDWidgetClass;
};
