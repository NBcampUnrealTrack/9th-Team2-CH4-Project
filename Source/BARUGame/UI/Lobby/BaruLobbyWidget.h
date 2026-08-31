// BaurLobbyWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/BaruActivatableWidget.h"

#include "BaruLobbyWidget.generated.h"

/**
 * 맵과 계약을 선택하고 팀원을 확인하는
 * 로비 화면의 C++ 기반 클래스
 */
UCLASS(Abstract, Blueprintable)
class BARUGAME_API UBaruLobbyWidget
	: public UBaruActivatableWidget
{
	GENERATED_BODY()
	
public:
	UBaruLobbyWidget(
		const FObjectInitializer& ObjectInitializer);
	
protected:
	virtual void NativeOnActivated() override;
	
	virtual void NativeOnDeactivated() override;
};