// BaruActivatableWidget.h

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Engine/EngineBaseTypes.h"

#include "BaruActivatableWidget.generated.h"

struct FUIInputConfig;

/**
 * Activatable Widget이 활성화됐을 때 사용할 입력 모드
 */
UENUM(BlueprintType)
enum class EBaruWidgetInputMode : uint8
{
	// 현재 입력 설정을 변경하지 않는다.
	Default,

	// 게임과 UI가 모두 입력을 받는다.
	GameAndMenu,

	// 게임만 입력을 받는다.
	Game,

	// UI만 입력 받는다.
	Menu
};

/**
 * 열고 닫거나 입력 포커스를 전환하는 UI의 공통 기반 클래스.
 *
 * 예:
 * - ESC 메뉴
 * - 인벤토리
 * - 설정
 * - 상점
 * - 정산 화면
 * - 확인 팝업
 */
UCLASS(Abstract, Blueprintable)
class BARUGAME_API UBaruActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	/**
	 * 이 위젯이 활성화될 때 Common UI가 적용할
	 * 게임/UI 입력 모드를 반환한다
	 */
	virtual TOptional<FUIInputConfig>
	GetDesiredInputConfig() const override;

protected:
	virtual void NativeOnActivated() override;

	virtual void NativeOnDeactivated() override;

	/**
	 * 이 위젯이 활성화됐을 때 사용할 입력 모드
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "BARU|UI|Input"
	)
	EBaruWidgetInputMode InputConfig =
		EBaruWidgetInputMode::Default;

	/**
	 * Game 또는 GameAndMenu 모드일 때 적용할
	 * 마우스 캡처 방식
	 */
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "BARU|UI|Input"
		)
	EMouseCaptureMode GameMouseCaptureMode =
		EMouseCaptureMode::CapturePermanently;

	// UI 활성화 전에 표시되면 커서 상태
	bool bPreviousMouseCursorVisible = false;

	// 이전 커서 상태를 정상적으로 저장했는지 여부
	bool bHasSavedMouseCursorState = false;
};
