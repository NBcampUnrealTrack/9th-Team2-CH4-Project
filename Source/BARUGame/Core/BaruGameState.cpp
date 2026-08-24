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

    DOREPLIFETIME(ABaruGameState, AlivePlayerCount);
    DOREPLIFETIME(ABaruGameState, TeamScrapValue);
}


// [Server Only] Setter

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


// Multicast RPC

void ABaruGameState::Multicast_BroadcastNotification_Implementation(const FText& MessageText, float DisplayDuration)
{
    BARU_NET_LOG(this, LogBaruUI, Log, TEXT("Global Notification: %s (Duration: %.1f)"), *MessageText.ToString(), DisplayDuration);
    OnGlobalNotificationReceived.Broadcast(MessageText, DisplayDuration);
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