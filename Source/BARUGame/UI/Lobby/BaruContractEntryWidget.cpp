// BaruContractEntryWidget.cpp

#include "UI/Lobby/BaruContractEntryWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

#include "BaruLog.h"
#include "UI/Lobby/BaruContractListItemData.h"

void UBaruContractEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	if (IsValid(Button_SelectContract))
	{
		Button_SelectContract->OnClicked.AddDynamic(
			this,
			&ThisClass::HandleSelectContractClicked
			);
	}
}

void UBaruContractEntryWidget::NativeOnListItemObjectSet(
	UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(
		ListItemObject
		);
	
	ItemData =
		Cast<UBaruContractListItemData>(ListItemObject);
	
	if (!IsValid(ItemData))
	{
		BARU_LOG(
			LogBaruUI,
			Error,
			TEXT("계약 ListView Item Data 형식이 올바르지 않습니다.")
			);
		
		return;
	}
	
	if (IsValid(Text_ContractName))
	{
		Text_ContractName->SetText(
			ItemData->ContractName
			);
	}
	
	if (IsValid(Text_MapName))
	{
		Text_MapName->SetText(
			ItemData->MapName
			);
	}
	
	if (IsValid(Text_Difficulty))
	{
		Text_Difficulty->SetText(
			ItemData->Difficulty
			);
	}
	
	if (IsValid(Text_Reward))
	{
		Text_Reward->SetText(
			ItemData->RewardText
			);
	}
	
	if (IsValid(Image_ContractThumbnail))
	{
		if (IsValid(ItemData->Thumbnail.Get()))
		{
			Image_ContractThumbnail->SetBrushFromTexture(
				ItemData->Thumbnail.Get()
				);
			
			Image_ContractThumbnail->SetVisibility(
				ESlateVisibility::Visible
				);
		}
		else
		{
			Image_ContractThumbnail->SetVisibility(
				ESlateVisibility::Collapsed
				);
		}
	}
}

void UBaruContractEntryWidget::HandleSelectContractClicked()
{
	if (!IsValid(ItemData))
	{
		return;
	}
	
	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("계약 항목을 선택했습니다. Contract=%s, Map=%s"),
		*ItemData->ContractName.ToString(),
		*ItemData->TargetMapURL
		);
	
	ItemData->RequestSelection();
}