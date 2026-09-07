// BaruContractEntryWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "UI/Foundation/BaruCommonUserWidget.h"

#include "BaruContractEntryWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UBaruContractListItemData;

/**
 * 계약 한 건을 화면에 표시하는 ListView Entry Widget
 */
UCLASS(Abstract, Blueprintable)
class BARUGAME_API UBaruContractEntryWidget 
	: public UBaruCommonUserWidget,
	public IUserObjectListEntry
{
	GENERATED_BODY()
	
protected:
	virtual void NativeOnInitialized() override;
	
	virtual void NativeOnListItemObjectSet(
		UObject* ListItemObject) override;
	
	UFUNCTION()
	void HandleSelectContractClicked();
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby|Contract")
	TObjectPtr<UButton> Button_SelectContract;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby|Contract")
	TObjectPtr<UImage> Image_ContractThumbnail;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby|Contract")
	TObjectPtr<UTextBlock> Text_ContractName;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby|Contract")
	TObjectPtr<UTextBlock> Text_MapName;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|LObby|Contract")
	TObjectPtr<UTextBlock> Text_Difficulty;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby|Contract")
	TObjectPtr<UTextBlock> Text_Reward;
	
	UPROPERTY(Transient)
	TObjectPtr<UBaruContractListItemData> ItemData;
};