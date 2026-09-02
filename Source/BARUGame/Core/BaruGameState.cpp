#include "Core/BaruGameState.h"
#include "Net/UnrealNetwork.h"
#include "BaruLog.h"

ABaruGameState::ABaruGameState()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetNetUpdateFrequency(10.0f);
}

void ABaruGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ABaruGameState, MatchState);
    DOREPLIFETIME(ABaruGameState, RemainingRaidTime);
    DOREPLIFETIME(ABaruGameState, AlivePlayerCount);
    DOREPLIFETIME(ABaruGameState, TeamScrapValue);
    DOREPLIFETIME(ABaruGameState, PlayersInExtractionZoneCount);
}


// [Server Only] Setter

void ABaruGameState::SetMatchState(EBaruMatchState NewState)
{
    if (!HasAuthority() || MatchState == NewState) return;

    MatchState = NewState;
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("MatchState Changed: %d"), static_cast<int32>(MatchState));
    OnRep_MatchState();
}

void ABaruGameState::SetRemainingRaidTime(int32 NewTime)
{
    if (!HasAuthority() || RemainingRaidTime == NewTime) return;

    RemainingRaidTime = FMath::Max(NewTime, 0);
    OnRep_RemainingRaidTime();
}

void ABaruGameState::SetAlivePlayerCount(int32 NewCount)
{
    if (!HasAuthority() || AlivePlayerCount == NewCount)
    {
        return;
    }

    AlivePlayerCount = FMath::Max(NewCount, 0);
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("AlivePlayerCount Changed: %d"), AlivePlayerCount);
    OnRep_AlivePlayerCount(); // Listen Server 및 서버 측 UI 즉시 갱신
}

void ABaruGameState::SetTeamScrapValue(int32 NewValue)
{
    if (!HasAuthority() || TeamScrapValue == NewValue)
    {
        return;
    }

    TeamScrapValue = FMath::Max(NewValue, 0);
    BARU_NET_LOG(this, LogBaruItem, Log, TEXT("TeamScrapValue Changed: %d"), TeamScrapValue);
    OnRep_TeamScrapValue();
}

void ABaruGameState::SetPlayersInExtractionZoneCount(int32 NewCount)
{
    if (!HasAuthority() || PlayersInExtractionZoneCount == NewCount) return;

    PlayersInExtractionZoneCount = FMath::Max(NewCount, 0);
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Players In Extraction Zone: %d / %d"), PlayersInExtractionZoneCount, AlivePlayerCount);
    OnRep_PlayersInExtractionZoneCount();
}

// Multicast RPC

void ABaruGameState::Multicast_BroadcastNotification_Implementation(const FText& MessageText, float DisplayDuration)
{
    BARU_NET_LOG(this, LogBaruUI, Log, TEXT("Global Notification: %s (Duration: %.1f)"), *MessageText.ToString(), DisplayDuration);
    OnGlobalNotificationReceived.Broadcast(MessageText, DisplayDuration);
}

void ABaruGameState::Multicast_BroadcastPing_Implementation(FVector PingLocation, EBaruPingType PingType)
{
    BARU_NET_LOG(this, LogBaruNet, Verbose, TEXT("Ping Broadcast: Type %d at %s"), static_cast<int32>(PingType), *PingLocation.ToString());
    OnPingReceived.Broadcast(PingLocation, PingType);
}

// RepNotifies

void ABaruGameState::OnRep_MatchState()
{
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("OnRep_MatchState: %d"), static_cast<int32>(MatchState));
    OnMatchStateChanged.Broadcast(MatchState);
}

void ABaruGameState::OnRep_RemainingRaidTime()
{
    OnRaidTimerUpdated.Broadcast(RemainingRaidTime);
}

void ABaruGameState::OnRep_AlivePlayerCount()
{
    BARU_NET_LOG(this, LogBaruSession, Verbose, TEXT("OnRep_AlivePlayerCount: %d"), AlivePlayerCount);
    OnAlivePlayerCountChanged.Broadcast(AlivePlayerCount);
}

void ABaruGameState::OnRep_TeamScrapValue()
{
    BARU_NET_LOG(this, LogBaruItem, Verbose, TEXT("OnRep_TeamScrapValue: %d"), TeamScrapValue);
    OnTeamScrapValueChanged.Broadcast(TeamScrapValue);
}

void ABaruGameState::OnRep_PlayersInExtractionZoneCount()
{
    OnExtractionPlayerCountChanged.Broadcast(PlayersInExtractionZoneCount, AlivePlayerCount);
}