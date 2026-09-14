// BaruInventoryTooltipWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Gameplay/Items/DataTypes/BaruItemData.h"
#include "BaruInventoryTooltipWidget.generated.h"

class UImage;
class UTextBlock;

UCLASS()
class BARUGAME_API UBaruInventoryTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeInfo(const FItemData& InData, int32 InQuantity);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	virtual void NativeTick(
		const FGeometry& MyGeometry,
		float InDeltaTime) override;

private:
	void RefreshInfo();
	void UpdatePlacement();

	UPROPERTY(Transient)
	FItemData DisplayData;

	int32 DisplayQuantity = 0;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Text_Name;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Text_Info;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Text_Description;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Text_Story;
};