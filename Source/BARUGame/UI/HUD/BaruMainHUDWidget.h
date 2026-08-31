// BaruMainHUDWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/BaruActivatableWidget.h"

#include "BaruMainHUDWidget.generated.h"

/**
 * 플레이 중 항상 표시되는 Main HUD의 C++ 기반 클래스,
 *
 * 이후 체력, 정신력, 크로스헤어, 상호작용 안내 등의
 * HUD 요소를 포함하는 최상위 위젯으로 사용한다.
 */
UCLASS(Abstract, Blueprintable)
class BARUGAME_API UBaruMainHUDWidget
	: public UBaruActivatableWidget
{
	GENERATED_BODY()

public:
	UBaruMainHUDWidget(
		const FObjectInitializer& ObjectInitializer);
	
	// [08.30] CommonUI가 포커스를 요구하지 않도록 비활성화, 무조건 1인칭 Game 전용 모드 반환
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override
	{
		return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently, true);
	}

protected:
	/**
	 * Main HUD가 활성화 될 때 호출된다.
	 *
	 * 이후 ViewModel 연결, Delegate 등록,
	 * 초기 데이터 갱신 등에 사용한다.
	 */

	virtual void NativeOnActivated() override;

	/**
	 * Main HUD가 비활성화될 때 호출된다.
	 *
	 * 이후 Delegate와 Listener 해제 등에 사용한다.
	 */
	virtual void NativeOnDeactivated() override;
	
protected:
	// [08.30] 포커스 타깃을 nullptr로 돌려 CommonUI의 Slate 포커스 강탈 방지
	virtual UWidget* NativeGetDesiredFocusTarget() const override { return nullptr; }
};
