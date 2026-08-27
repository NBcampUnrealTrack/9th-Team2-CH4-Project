// BaruPrimaryGameLayout.h

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/Foundation/BaruCommonUserWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#include "BaruPrimaryGameLayout.generated.h"

class UCommonActivatableWidget;

/**
 * 로컬 플레이어 UI 전체를 담는 최상위 레이아웃
 *
 * GameLayer	 : Main HUD
 * GameMenuLayer : 인벤토리, ESC 메뉴, 설정
 * ModalLayer	 : 확인창처럼 가장 위에 표시되는 화면
 */
UCLASS(Abstract, Blueprintable)
class BARUGAME_API UBaruPrimaryGameLayout : public UBaruCommonUserWidget
{
	GENERATED_BODY()
	
public:
	/** 지정한 UI 레이어의 Stack에 위젯을 추가
	 * 
	 * @param LayerTag 위젯을 추가할 UI 레이어
	 * @param WidgetClass 생성할 Common Activatable Widget 클래스
	 * @return 생성되어 Stack에 추가된 위젯. 실패하면 nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "BARU|UI")
	UCommonActivatableWidget* PushWidgetToLayer(
		UPARAM(meta = (Categories = "UI.Layer"))
		FGameplayTag LayerTag,
		TSubclassOf<UCommonActivatableWidget> WidgetClass
		);
	
	/**
	 * 지정한 UI 레이어에서 현재 가장 위에 있는 위젯을 닫는다.
	 * 
	 * @return 닫을 위젯이 있었다면 true
	 */
	UFUNCTION(BlueprintCallable, Category = "BARU|UI")
	bool PopWidgetFromLayer(
		UPARAM(meta = (Categories = "UI.Layer"))
		FGameplayTag LayerTag
		);

protected:
	// Main HUD가 들어가는 가장 아래 레이어
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI")
	TObjectPtr<UCommonActivatableWidgetStack> GameLayer;

	// ESC 메뉴, 인벤토리 등이 들어가는 중간 레이어
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI")
	TObjectPtr<UCommonActivatableWidgetStack> GameMenuLayer;

	// 확인창과 중요한 팝업이 들어가는 최상위 레이어
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI")
	TObjectPtr<UCommonActivatableWidgetStack> ModalLayer;
	
private:
	/**
	 * Gameplay Tag에 대응하는 실제 Widget Stack을 찾는다
	 */
	UCommonActivatableWidgetStack* GetLayerStack(
		FGameplayTag LayerTag
		) const;
};
