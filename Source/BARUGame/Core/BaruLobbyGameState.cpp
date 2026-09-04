#include "Core/BaruLobbyGameState.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "BaruLog.h"

ABaruLobbyGameState::ABaruLobbyGameState()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetNetUpdateFrequency(10.0f);
}

void ABaruLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaruLobbyGameState, bAllPlayersReady);
	DOREPLIFETIME(ABaruLobbyGameState, SelectedTargetMapURL);
}

void ABaruLobbyGameState::SetAllPlayersReady(bool bInAllReady)
{
	if (!HasAuthority() || bAllPlayersReady == bInAllReady)
	{
		return;
	}

	bAllPlayersReady = bInAllReady;
	BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Lobby bAllPlayersReady Changed: %d"), bAllPlayersReady);
	
	OnRep_AllPlayersReady();
}

void ABaruLobbyGameState::SetSelectedTargetMapURL(const FString& InMapURL)
{
	if (!HasAuthority() || SelectedTargetMapURL == InMapURL)
	{
		return;
	}

	SelectedTargetMapURL = InMapURL;
	BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Lobby SelectedTargetMapURL Changed: %s"), *SelectedTargetMapURL);

	OnRep_SelectedTargetMapURL();
}

void ABaruLobbyGameState::OnRep_AllPlayersReady()
{
	BARU_NET_LOG(this, LogBaruSession, Verbose, TEXT("OnRep_AllPlayersReady: %d"), bAllPlayersReady);
	OnAllPlayersReadyChanged.Broadcast(bAllPlayersReady);
}

void ABaruLobbyGameState::OnRep_SelectedTargetMapURL()
{
	BARU_NET_LOG(this, LogBaruSession, Verbose, TEXT("OnRep_SelectedTargetMapURL: %s"), *SelectedTargetMapURL);
	OnTargetMapChanged.Broadcast(SelectedTargetMapURL);
}

void ABaruLobbyGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);
	BARU_NET_LOG(this, LogBaruSession, Log, TEXT("PlayerState Added to Lobby: %s"), *GetNameSafe(PlayerState));
	OnLobbyPlayerArrayUpdated.Broadcast();
}

void ABaruLobbyGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);
	BARU_NET_LOG(this, LogBaruSession, Log, TEXT("PlayerState Removed from Lobby: %s"), *GetNameSafe(PlayerState));
	OnLobbyPlayerArrayUpdated.Broadcast();
}