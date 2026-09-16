#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/BaruCommonUserWidget.h"
#include "BaruSpectatorInfoWidget.generated.h"

class UTextBlock;
class UBorder;
class ABaruPlayerController;

/**
 * [신규] 관전 중인 대상의 닉네임과 조작 안내를 표시하는 독립 위젯
 */
UCLASS(Abstract, Blueprintable)
class BARUGAME_API UBaruSpectatorInfoWidget : public UBaruCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Border_SpectatorInfo;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SpectatorTargetName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SpectatorGuide;

	UFUNCTION()
	void HandleSpectatorTargetChanged(const FString& TargetPlayerName);

private:
	void InitControllerBinding();
	void UpdateSpectatorDisplay(const FString& TargetPlayerName);

	UPROPERTY(Transient)
	TWeakObjectPtr<ABaruPlayerController> CachedPlayerController;

	FTimerHandle RetryInitTimerHandle;
};