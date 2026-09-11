#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BaruItemFocusWidget.generated.h"

class ABaruBaseItem;
class APawn;
class UBorder;
class UCanvasPanelSlot;
class UTextBlock;

// C++에서 화면 구성을 만듭니다. 별도 WBP/BindWidget 설정이 필요 없습니다.
UCLASS()
class BARUGAME_API UBaruItemFocusWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void ShowItem(const ABaruBaseItem* Item, APawn* Viewer);
    void HideItem();
    void ConfigureCard(FVector2D Anchor, float Width);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UBorder> Card;

    UPROPERTY(Transient)
    TObjectPtr<UCanvasPanelSlot> CardSlot;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> NameText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ValueText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> CategoryText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> DetailText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PromptText;
};
