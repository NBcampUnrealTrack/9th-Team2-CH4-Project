// BaruTitleWidge.h

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/BaruActivatableWidget.h"

#include "BaruTitleWidget.generated.h"

class UButton;

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
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_StartGame;
	
	// 게임 종료 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_QuitGame;
};