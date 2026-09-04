#include "UI/Lobby/BaruLobbyPlayerListItemData.h"

void UBaruLobbyPlayerListItemData::Initialize(
	const FString& InPlayerName,
	bool bInIsHost,
	bool bInIsReady,
	bool bInIsLocalPlayer)
{
	PlayerName = InPlayerName;
	bIsHost = bInIsHost;
	bIsReady = bInIsReady;
	bIsLocalPlayer = bInIsLocalPlayer;
}
