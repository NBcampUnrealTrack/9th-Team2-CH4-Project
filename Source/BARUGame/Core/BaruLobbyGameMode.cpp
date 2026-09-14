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

void ABaruLobbyGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (!CachedLobbyGameState)
    {
        CachedLobbyGameState = GetGameState<ABaruLobbyGameState>();
    }

    // 로비 레벨 로드 시 솔로 플레이어 즉시 시작 가능 상태로 1회 강제 평가
    OnPlayerReadyStatusChanged();
}

void ABaruLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    // 신규 접속 시 공용 헬퍼 호출
    InitializeLobbyPlayerState(NewPlayer);
}

void ABaruLobbyGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
    Super::HandleSeamlessTravelPlayer(C);

    if (APlayerController* PC = Cast<APlayerController>(C))
    {
        InitializeLobbyPlayerState(PC);
    }
}

void ABaruLobbyGameMode::PostSeamlessTravel()
{
    Super::PostSeamlessTravel();

    OnPlayerReadyStatusChanged();
}

void ABaruLobbyGameMode::InitializeLobbyPlayerState(APlayerController* PC)
{
    if (!IsValid(PC)) return;

    BARU_NET_LOG(PC, LogBaruSession, Log, TEXT("Lobby Player Initialized: %s"), *PC->GetName());

    if (ABaruPlayerState* PS = PC->GetPlayerState<ABaruPlayerState>())
    {
        PS->ResetPlayerStatusAndInventory();

        PS->OnReadyStatusChanged.RemoveDynamic(this, &ABaruLobbyGameMode::HandlePlayerReadyStatusChanged);
        PS->OnReadyStatusChanged.AddDynamic(this, &ABaruLobbyGameMode::HandlePlayerReadyStatusChanged);
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

    OnPlayerReadyStatusChanged();
}

void ABaruLobbyGameMode::Logout(AController* Exiting)
{
    if (IsValid(Exiting))
    {
        BARU_NET_LOG(Exiting, LogBaruSession, Log, TEXT("Lobby Player Logged Out: %s"), *Exiting->GetName());
        
        if (ABaruPlayerState* PS = Exiting->GetPlayerState<ABaruPlayerState>())
        {
            PS->OnReadyStatusChanged.RemoveDynamic(this, &ABaruLobbyGameMode::HandlePlayerReadyStatusChanged);
        }

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

FString ABaruLobbyGameMode::GetTargetRaidMap() const
{
    if (CachedLobbyGameState && !CachedLobbyGameState->GetSelectedTargetMapURL().IsEmpty())
    {
        return CachedLobbyGameState->GetSelectedTargetMapURL();
    }

    return DefaultRaidMapURL;
}

void ABaruLobbyGameMode::HandlePlayerReadyStatusChanged(bool /*bIsReady*/)
{
    OnPlayerReadyStatusChanged();
}

void ABaruLobbyGameMode::OnPlayerReadyStatusChanged()
{
    if (!CachedLobbyGameState)
    {
        CachedLobbyGameState = GetGameState<ABaruLobbyGameState>();
    }

    // 방장 제외 인원 집계
    int32 GuestPlayerCount = 0;
    int32 ReadyGuestCount = 0;

    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        APlayerController* PC = Iterator->Get();
        if (!IsValid(PC) || PC->IsPendingKillPending())
        {
            continue;
        }

        // 리슨 서버의 호스트(방장 머신) 컨트롤러는 준비 대상에서 제외
        if (PC->IsLocalController())
        {
            continue;
        }

        GuestPlayerCount++;
        if (const ABaruPlayerState* PS = PC->GetPlayerState<ABaruPlayerState>())
        {
            if (PS->IsReady())
            {
                ReadyGuestCount++;
            }
        }
    }

    // 게스트가 없는 1인 방(솔로 플레이)이면 즉시 출발 가능(true),
    // 게스트가 1명 이상 접속해 있다면 게스트 전원이 레디해야 true 판정
    const bool bAllReady = (GuestPlayerCount == 0) || (ReadyGuestCount == GuestPlayerCount);

    // GameState를 통해 모든 클라이언트로 복제 전파
    if (CachedLobbyGameState)
    {
        CachedLobbyGameState->SetAllPlayersReady(bAllReady);
    }

    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Lobby Ready Status: %d / %d Guests Ready (bAllReady: %d)"),
        ReadyGuestCount, GuestPlayerCount, bAllReady);
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
    
    int32 DotIndex = INDEX_NONE;
    if (DestinationMap.FindChar(TEXT('.'), DotIndex))
    {
        DestinationMap.LeftInline(DotIndex);
    }

    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Initiating Seamless Travel to: %s"), *DestinationMap);
    GetWorld()->ServerTravel(DestinationMap + TEXT("?listen"));
}