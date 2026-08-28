// BaruUIManagerSubsystem.h

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "GameplayTagContainer.h"
#include "Subsystems/LocalPlayerSubsystem.h"

#include "BaruUIManagerSubsystem.generated.h"

class UBaruPrimaryGameLayout;

/**
 * 로컬 플레이어 UI 요청을 관리하는 공용 진입점
 *
 * 팀원은 PrimaryGameLayout이나 Layer Stack에 직접 접근하지 않고
 * 이 Subsystem을 통해 UI를 열고 닫는다.
 */
UCLASS()
class BARUGAME_API UBaruUIManagerSubsystem
	: public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * HUD가 생성한 PrimaryGameLayout을 등록한다.
	 */
	void RegisterPrimaryLayout(
		UBaruPrimaryGameLayout* InPrimaryLayout
		);

	/**
	 * 등록된 PrimaryGameLayout을 해제한다.
	 */
	void UnregisterPrimaryLayout(
		UBaruPrimaryGameLayout* InPrimaryLayout
		);

	/**
	 * 지정한 레이어에 위젯을 추가한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "BARU|UI")
	UCommonActivatableWidget* PushWidgetToLayer(
		UPARAM(meta = (Categories = "UI.Layer"))
		FGameplayTag LayerTag,
		TSubclassOf<UCommonActivatableWidget> WidgetClass
		);

	/**
	 * 지정한 레이어의 가장 위에 있는 위젯을 닫는다.
	 */
	UFUNCTION(BlueprintCallable, Category = "BARU|UI")
	bool PopWidgetFromLayer(
		UPARAM(meta = (Categories = "UI.Layer"))
		FGameplayTag LayerTag
		);

protected:
	/**
	 * LocalPlayerSubsystem이 종료될 때 호출된다.
	 */
	virtual void Deinitialize() override;

private:
	/**
	 * 현재 로컬 플레이어의 PrimaryGameLayout.
	 *
	 * UI Manager는 Layout의 수명을 소유하지 않으므로
	 * 약한 참조를 보관한다.
	 */
	UPROPERTY(Transient)
	TWeakObjectPtr<UBaruPrimaryGameLayout> PrimaryLayout;
};