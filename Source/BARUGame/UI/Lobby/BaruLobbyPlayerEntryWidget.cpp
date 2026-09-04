#include "UI/Lobby/BaruLobbyPlayerEntryWidget.h"

#include "Components/TextBlock.h"
#include "UI/Lobby/BaruLobbyPlayerListItemData.h"

void UBaruLobbyPlayerEntryWidget::NativeOnListItemObjectSet(
	UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UBaruLobbyPlayerListItemData* PlayerItem =
		Cast<UBaruLobbyPlayerListItemData>(ListItemObject);

	if (!IsValid(PlayerItem) ||
		!IsValid(Text_PlayerName) ||
		!IsValid(Text_PlayerRole) ||
		!IsValid(Text_ReadyState))
	{
		return;
	}

	FString DisplayName = PlayerItem->PlayerName;
	if (PlayerItem->bIsLocalPlayer)
	{
		DisplayName += TEXT(" (나)");
	}

	Text_PlayerName->SetText(FText::FromString(DisplayName));
	Text_PlayerRole->SetText(FText::FromString(
		PlayerItem->bIsHost ? TEXT("방장") : TEXT("참가자")));
	Text_ReadyState->SetText(FText::FromString(
		PlayerItem->bIsHost
			? TEXT("-")
			: (PlayerItem->bIsReady ? TEXT("준비 완료") : TEXT("준비 안 됨"))));
}
