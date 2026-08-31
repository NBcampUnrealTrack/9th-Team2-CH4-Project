// BaruHUD.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GameplayTagContainer.h"

#include "BaruHUD.generated.h"

class UBaruPrimaryGameLayout;
class UCommonActivatableWidget;

/**
 * 로컬 플레이어의 Primary Game Layout 생성과 초기 위젯 표시를 담당한다.
 * Dedicated Server에서는 UI를 생성하지 않는다.
 */
UCLASS(Blueprintable)
class BARUGAME_API ABaruHUD : public AHUD
{
	GENERATED_BODY()

public:
	ABaruHUD();

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 화면에 생성할 Primary Game Layout Blueprint 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|UI")
	TSubclassOf<UBaruPrimaryGameLayout> PrimaryGameLayoutClass;

	/**
	 * PrimaryGameLayout이 준비된 후 처음 열 위젯 클래스
	 *
	 * 게임 플레이에서는 WBP_MainHUD,
	 * 메인 메뉴에서는 WBP_Title을 설정한다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|UI")
	TSubclassOf<UCommonActivatableWidget> InitialWidgetClass;

	/**
	 * InitialWidgetClass를 추가할 UI 레이어
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|UI", meta = (Categories = "UI.Layer"))
	FGameplayTag InitialWidgetLayerTag;

	// 실행 중 생성된 로컬 플레이어의 Primary Game Layout
	UPROPERTY(Transient, BlueprintReadOnly, Category = "BARU|UI")
	TObjectPtr<UBaruPrimaryGameLayout> PrimaryGameLayout;
};
