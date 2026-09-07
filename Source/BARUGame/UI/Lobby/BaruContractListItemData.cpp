// BaruContractListItemData.cpp

#include "UI/Lobby/BaruContractListItemData.h"

#include "Engine/Texture2D.h"

void UBaruContractListItemData::Initialize(
	const FText& InContractName,
	const FText& InMapName,
	const FText& InDifficulty,
	const FText& InRewardText,
	const FString& InTargetMapURL,
	UTexture2D* InThumbnail)
{
	ContractName = InContractName;
	MapName = InMapName;
	Difficulty = InDifficulty;
	RewardText = InRewardText;
	TargetMapURL = InTargetMapURL;
	Thumbnail = InThumbnail;
}

void UBaruContractListItemData::RequestSelection()
{
	OnSelected.Broadcast(this);
}