// BaruSharedAssetWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/BaruCommonUserWidget.h"
#include "BaruSharedAssetWidget.generated.h"

class UTextBlock;
class ABaruGameState;

UCLASS(Abstract)
class BARUGAME_API UBaruSharedAssetWidget
	: public UBaruCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// WBP_BaruEquipment의 동일한 이름 TextBlock과 연결
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SharedAsset;

	UFUNCTION()
	void HandleTeamScrapValueChanged(int32 NewTeamValue);

private:
	void BindGameState();

	UPROPERTY(Transient)
	TObjectPtr<ABaruGameState> BoundGameState;
};