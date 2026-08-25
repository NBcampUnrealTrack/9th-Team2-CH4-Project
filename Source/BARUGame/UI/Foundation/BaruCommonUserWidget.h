// BaruCommonUserWidget.h

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"

#include "BaruCommonUserWidget.generated.h"

/**
 * Main HUD 및 항상 표시되는 UI의 공통 기반 클래스,
 *
 * 예:
 * - Main HUD
 * - 체력 / 정신력
 * - 크로스헤어
 * - 퀵슬롯
 * - 상호작용 안내
 */
UCLASS(Abstract, Blueprintable)
class BARUGAME_API UBaruCommonUserWidget : public UCommonUserWidget
{
	GENERATED_BODY()
};
