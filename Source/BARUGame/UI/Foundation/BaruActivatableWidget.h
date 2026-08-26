// BaruActivatableWidget.h

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"

#include "BaruActivatableWidget.generated.h"

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
};
