// BaruPrimaryGameLayout.h

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/BaruCommonUserWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#include "BaruPrimaryGameLayout.generated.h"

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
};
