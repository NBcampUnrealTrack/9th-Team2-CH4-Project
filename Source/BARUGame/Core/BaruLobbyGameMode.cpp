#include "Core/BaruLobbyGameMode.h"
#include "Core/BaruLobbyGameState.h"
#include "Player/BaruPlayerController.h"
#include "Player/BaruPlayerState.h"
#include "Character/BaruCharacter.h"
#include "Engine/World.h"
#include "BaruLog.h"

ABaruLobbyGameMode::ABaruLobbyGameMode()
{
    PrimaryActorTick.bCanEverTick = false;
    bUseSeamlessTravel = true;

    GameStateClass = ABaruLobbyGameState::StaticClass();
    PlayerControllerClass = ABaruPlayerController::StaticClass();
    PlayerStateClass = ABaruPlayerState::StaticClass();
    DefaultPawnClass = ABaruCharacter::StaticClass();

}

void ABaruLobbyGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    BARU_LOG(LogBaruSession, Log, TEXT("ABaruLobbyGameMode Initialized on Map: %s"), *MapName);
}

void ABaruLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (IsValid(NewPlayer))
    {
        BARU_NET_LOG(NewPlayer, LogBaruSession, Log, TEXT("Lobby Player Logged In: %s"), *NewPlayer->GetName());
    }

    if (!CachedLobbyGameState)
    {
        CachedLobbyGameState = GetGameState<ABaruLobbyGameState>();
        if (CachedLobbyGameState && CachedLobbyGameState->GetSelectedTargetMapURL().IsEmpty())
        {
            CachedLobbyGameState->SetSelectedTargetMapURL(DefaultRaidMapURL);
        }
    }

    if (CachedLobbyGameState)
    {
        CachedLobbyGameState->SetAlivePlayerCount(GetNumPlayers());
        CachedLobbyGameState->SetMatchState(EBaruMatchState::WaitingToStart);
    }

    // 신규 인원 접속 시 레디 상태 재평가
    OnPlayerReadyStatusChanged();
}

void ABaruLobbyGameMode::Logout(AController* Exiting)
{
    if (IsValid(Exiting))
    {
        BARU_NET_LOG(Exiting, LogBaruSession, Log, TEXT("Lobby Player Logged Out: %s"), *Exiting->GetName());

        if (APawn* ControlledPawn = Exiting->GetPawn())
        {
            ControlledPawn->Destroy();
        }
    }

    Super::Logout(Exiting);

    if (!CachedLobbyGameState)
    {
        CachedLobbyGameState = GetGameState<ABaruLobbyGameState>();
    }

    if (CachedLobbyGameState)
    {
        CachedLobbyGameState->SetAlivePlayerCount(GetNumPlayers());
    }

    // 인원 이탈 시 레디 상태 재평가
    OnPlayerReadyStatusChanged();
}

void ABaruLobbyGameMode::SetTargetRaidMap(const FString& InMapName)
{
    if (!CachedLobbyGameState)
    {
        CachedLobbyGameState = GetGameState<ABaruLobbyGameState>();
    }

    if (CachedLobbyGameState)
    {
        CachedLobbyGameState->SetSelectedTargetMapURL(InMapName);
    }

    BARU_LOG(LogBaruSession, Log, TEXT("Lobby Target Map Changed: %s"), *InMapName);
}

void ABaruLobbyGameMode::OnPlayerReadyStatusChanged()
{
    if (!CachedLobbyGameState)
    {
        CachedLobbyGameState = GetGameState<ABaruLobbyGameState>();
    }

    int32 TotalPlayers = 0;
    int32 ReadyPlayers = 0;

    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        APlayerController* PC = Iterator->Get();
        if (IsValid(PC) && !PC->IsPendingKillPending())
        {
            TotalPlayers++;
            if (const ABaruPlayerState* PS = PC->GetPlayerState<ABaruPlayerState>())
            {
                if (PS->IsReady())
                {
                    ReadyPlayers++;
                }
            }
        }
    }

    // 최소 1명 이상 접속 중이고 모든 인원이 레디를 마쳤는지 검사
    const bool bAllReady = (TotalPlayers > 0) && (ReadyPlayers == TotalPlayers);

    // GameState를 통해 모든 클라이언트로 복제 전파
    if (CachedLobbyGameState)
    {
        CachedLobbyGameState->SetAllPlayersReady(bAllReady);
    }

    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Lobby Ready Status: %d / %d Ready (bAllReady: %d)"), ReadyPlayers, TotalPlayers, bAllReady);
}

void ABaruLobbyGameMode::StartGameRaid(const FString& OverrideTargetMapURL)
{
    if (!HasAuthority())
    {
        return;
    }

    FString DestinationMap = OverrideTargetMapURL;
    if (DestinationMap.IsEmpty() && CachedLobbyGameState)
    {
        DestinationMap = CachedLobbyGameState->GetSelectedTargetMapURL();
    }
    if (DestinationMap.IsEmpty())
    {
        DestinationMap = DefaultRaidMapURL;
    }

    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Initiating Seamless Travel to: %s"), *DestinationMap);
    GetWorld()->ServerTravel(DestinationMap + TEXT("?listen"));
}