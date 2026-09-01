#include "Core/BaruLobbyGameMode.h"
#include "Core/BaruGameState.h"
#include "Player/BaruPlayerController.h"
#include "Player/BaruPlayerState.h"
#include "Character/BaruCharacter.h"
#include "Engine/World.h"
#include "BaruLog.h"

ABaruLobbyGameMode::ABaruLobbyGameMode()
{
    PrimaryActorTick.bCanEverTick = false;

    // 세션 유지 및 PlayerState 보존을 위한 필수 설정
    bUseSeamlessTravel = true;

    GameStateClass = ABaruGameState::StaticClass();
    PlayerControllerClass = ABaruPlayerController::StaticClass();
    PlayerStateClass = ABaruPlayerState::StaticClass();
    DefaultPawnClass = ABaruCharacter::StaticClass();

    SelectedTargetMapURL = DefaultRaidMapURL;
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

    if (!CachedBaruGameState)
    {
        CachedBaruGameState = GetGameState<ABaruGameState>();
    }

    if (CachedBaruGameState)
    {
        CachedBaruGameState->SetAlivePlayerCount(GetNumPlayers());
        CachedBaruGameState->SetMatchState(EBaruMatchState::WaitingToStart);
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

    if (CachedBaruGameState)
    {
        CachedBaruGameState->SetAlivePlayerCount(GetNumPlayers());
    }

    // 인원 이탈 시 레디 상태 재평가
    OnPlayerReadyStatusChanged();
}

void ABaruLobbyGameMode::SetTargetRaidMap(const FString& InMapName)
{
    SelectedTargetMapURL = InMapName;
    BARU_LOG(LogBaruSession, Log, TEXT("Lobby Target Map Changed: %s"), *SelectedTargetMapURL);
}

void ABaruLobbyGameMode::OnPlayerReadyStatusChanged()
{
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
    OnAllPlayersReadyStatusChanged.Broadcast(bAllReady);

    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Lobby Ready Status: %d / %d Ready (bAllReady: %d)"), ReadyPlayers, TotalPlayers, bAllReady);
}

void ABaruLobbyGameMode::StartGameRaid(const FString& OverrideTargetMapURL)
{
    const FString DestinationMap = OverrideTargetMapURL.IsEmpty() ? SelectedTargetMapURL : OverrideTargetMapURL;
    
    if (DestinationMap.IsEmpty())
    {
        BARU_LOG(LogBaruSession, Error, TEXT("StartGameRaid Failed: Destination Map URL is empty."));
        return;
    }

    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Initiating Seamless Travel to: %s"), *DestinationMap);

    // TransitionMap을 경유하는 Seamless ServerTravel 실행
    GetWorld()->ServerTravel(DestinationMap + TEXT("?listen"));
}