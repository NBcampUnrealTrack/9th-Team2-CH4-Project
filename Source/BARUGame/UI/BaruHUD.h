// BaruHUD.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"

#include "BaruHUD.generated.h"

class UBaruMainHUDWidget;
class UBaruPrimaryGameLayout;

/**
 * 로컬 플레이어의 Primary Game Layout 생성과 보관을 담당한다
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

	// GameLayer에 추가할 Main HUD Blueprint 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|UI")
	TSubclassOf<UBaruMainHUDWidget> MainHUDWidgetClass;
	
	// 실행 중 생성된 로컬 플레이어의 Primary Game Layout
	UPROPERTY(Transient, BlueprintReadOnly, Category = "BARU|UI")
	TObjectPtr<UBaruPrimaryGameLayout> PrimaryGameLayout;
};
