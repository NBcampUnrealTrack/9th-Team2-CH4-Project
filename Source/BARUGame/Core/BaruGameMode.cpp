#include "Core/BaruGameMode.h"
#include "Core/BaruGameState.h"
#include "Player/BaruPlayerController.h"
#include "Player/BaruPlayerState.h"
#include "Character/BaruCharacter.h"
#include "Subsystems/BaruSaveGameSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "BaruLog.h"

ABaruGameMode::ABaruGameMode()
{
    PrimaryActorTick.bCanEverTick = false;

    // 인게임에서도 다음 탐사 또는 로비 복귀를 위해 Seamless Travel 활성화
    bUseSeamlessTravel = true;

    GameStateClass = ABaruGameState::StaticClass();
    PlayerControllerClass = ABaruPlayerController::StaticClass();
    PlayerStateClass = ABaruPlayerState::StaticClass();
    DefaultPawnClass = ABaruCharacter::StaticClass();
}

void ABaruGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    BARU_LOG(LogBaruSession, Log, TEXT("ABaruGameMode::InitGame on Ingame Map: %s"), *MapName);
}

void ABaruGameMode::BeginPlay()
{
    Super::BeginPlay();

    CachedBaruGameState = GetGameState<ABaruGameState>();

    // 인게임 진입 즉시 탐사 상태로 전환 및 레이드 타이머 시작
    SetMatchPhase(EBaruMatchState::InProgress);
    StartRaidTimer();
}

void ABaruGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (IsValid(NewPlayer))
    {
        BARU_NET_LOG(NewPlayer, LogBaruSession, Log, TEXT("Ingame Player Logged In: %s"), *NewPlayer->GetName());
    }

    UpdateAlivePlayerCount();
}

void ABaruGameMode::Logout(AController* Exiting)
{
    if (IsValid(Exiting))
    {
        BARU_NET_LOG(Exiting, LogBaruSession, Log, TEXT("Ingame Player Logged Out: %s"), *Exiting->GetName());

        if (APawn* ControlledPawn = Exiting->GetPawn())
        {
            ControlledPawn->Destroy();
        }
    }

    Super::Logout(Exiting);

    UpdateAlivePlayerCount();
    CheckTeamWipe();
}

void ABaruGameMode::SetMatchPhase(EBaruMatchState NewPhase)
{
    if (!CachedBaruGameState)
    {
        CachedBaruGameState = GetGameState<ABaruGameState>();
    }

    if (CachedBaruGameState)
    {
        CachedBaruGameState->SetMatchState(NewPhase);
    }
}

void ABaruGameMode::StartRaidTimer()
{
    if (CachedBaruGameState)
    {
        CachedBaruGameState->SetRemainingRaidTime(RaidDurationInSeconds);
    }

    GetWorldTimerManager().SetTimer(
        RaidCountdownTimerHandle,
        this,
        &ABaruGameMode::UpdateRaidCountdown,
        1.0f,
        true
    );
    BARU_LOG(LogBaruSession, Log, TEXT("Raid Countdown Timer Started: %d Seconds"), RaidDurationInSeconds);
}

void ABaruGameMode::UpdateRaidCountdown()
{
    if (!CachedBaruGameState) return;

    const int32 CurrentTime = CachedBaruGameState->GetRemainingRaidTime();
    if (CurrentTime > 0)
    {
        CachedBaruGameState->SetRemainingRaidTime(CurrentTime - 1);

        // 잔여 시간 60초 긴급 경고 공지
        if (CurrentTime == 60)
        {
            CachedBaruGameState->Multicast_BroadcastNotification(
                FText::FromString(TEXT("WARNING: Facility Shutdown in 60 Seconds!")), 5.0f);
        }
    }
    else
    {
        GetWorldTimerManager().ClearTimer(RaidCountdownTimerHandle);
        OnRaidTimeout();
    }
}

void ABaruGameMode::OnRaidTimeout()
{
    BARU_LOG(LogBaruSession, Warning, TEXT("Raid Time Expired! Facility Locked Down."));

    if (CachedBaruGameState)
    {
        CachedBaruGameState->Multicast_BroadcastNotification(
            FText::FromString(TEXT("TIME OUT: Facility Locked. Extraction Failed.")), 5.0f);
    }

    // 시간 초과 시 전원 탈출 실패 정산 실행
    ProcessSettlement(false);
}

void ABaruGameMode::OnExtractionZoneCountChanged(int32 InZoneCount)
{
    if (!CachedBaruGameState) return;

    CachedBaruGameState->SetPlayersInExtractionZoneCount(InZoneCount);

    const int32 AliveCount = CachedBaruGameState->GetAlivePlayerCount();
    if (AliveCount > 0 && InZoneCount >= AliveCount)
    {
        BARU_LOG(LogBaruSession, Log, TEXT("All Alive Players In Extraction Zone. Ready for Departure."));
        SetMatchPhase(EBaruMatchState::Extraction);
    }
}

void ABaruGameMode::RequestLevelTransition(const FString& TargetMapURL)
{
    if (TargetMapURL.IsEmpty()) return;

    if (GetWorldTimerManager().IsTimerActive(LevelTransitionTimerHandle)) return;

    PendingTargetMapURL = TargetMapURL;
    SetMatchPhase(EBaruMatchState::Extraction);

    // 모든 클라이언트에 안도 시네마틱 연출 브로드캐스트
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        if (ABaruPlayerController* BaruPC = Cast<ABaruPlayerController>(Iterator->Get()))
        {
            if (IsValid(BaruPC) && !BaruPC->IsPendingKillPending())
            {
                BaruPC->Client_PlayElevatorCinematic();
            }
        }
    }

    // 연출 시간 대기 후 실제 ServerTravel 실행
    GetWorldTimerManager().SetTimer(
        LevelTransitionTimerHandle,
        this,
        &ABaruGameMode::ExecuteServerTravel,
        TransitionDelayDuration,
        false
    );
}

void ABaruGameMode::ExecuteServerTravel()
{
    if (PendingTargetMapURL.IsEmpty())
    {
        BARU_LOG(LogBaruSession, Error, TEXT("ExecuteServerTravel Failed: Empty Target Map URL."));
        return;
    }

    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Executing ServerTravel to: %s"), *PendingTargetMapURL);
    GetWorld()->ServerTravel(PendingTargetMapURL + TEXT("?listen"));
}

void ABaruGameMode::UpdateAlivePlayerCount()
{
    if (!CachedBaruGameState)
    {
        CachedBaruGameState = GetGameState<ABaruGameState>();
    }

    if (!CachedBaruGameState) return;

    int32 CurrentAlive = 0;
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        APlayerController* PC = Iterator->Get();
        if (IsValid(PC) && !PC->IsPendingKillPending())
        {
            if (const ABaruPlayerState* PS = PC->GetPlayerState<ABaruPlayerState>())
            {
                if (PS->IsAlive())
                {
                    CurrentAlive++;
                }
            }
        }
    }

    CachedBaruGameState->SetAlivePlayerCount(CurrentAlive);
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Ingame Alive Player Count: %d"), CurrentAlive);
}

void ABaruGameMode::OnPlayerDied(AController* VictimController, AActor* KillerActor)
{
    BARU_NET_LOG(VictimController, LogBaruCombat, Log, TEXT("Player Died: %s (Killer: %s)"),
        VictimController ? *VictimController->GetName() : TEXT("None"),
        KillerActor ? *KillerActor->GetName() : TEXT("None"));

    UpdateAlivePlayerCount();

    if (APlayerController* VictimPC = Cast<APlayerController>(VictimController))
    {
        StartSpectating(VictimPC);
    }

    CheckTeamWipe();
}

void ABaruGameMode::StartSpectating(APlayerController* DeadController)
{
    if (!IsValid(DeadController) || DeadController->IsPendingKillPending()) return;

    DeadController->UnPossess();
    DeadController->StartSpectatingOnly();

    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        APlayerController* OtherPC = Iterator->Get();
        if (IsValid(OtherPC) && OtherPC != DeadController && !OtherPC->IsPendingKillPending())
        {
            if (const ABaruPlayerState* PS = OtherPC->GetPlayerState<ABaruPlayerState>())
            {
                if (PS->IsAlive() && OtherPC->GetPawn())
                {
                    DeadController->SetViewTargetWithBlend(OtherPC->GetPawn(), 1.0f);

                    if (!DeadController->IsLocalController())
                    {
                        DeadController->ClientSetViewTarget(OtherPC->GetPawn(), FViewTargetTransitionParams());
                    }
                    BARU_NET_LOG(DeadController, LogBaruSession, Log, TEXT("Spectating Target Set to: %s"), *OtherPC->GetName());
                    return;
                }
            }
        }
    }
}

void ABaruGameMode::CheckTeamWipe()
{
    if (CachedBaruGameState && CachedBaruGameState->GetAlivePlayerCount() <= 0)
    {
        BARU_NET_LOG(this, LogBaruSession, Warning, TEXT("Team wiped. Processing Failure Settlement."));

        GetWorldTimerManager().ClearTimer(LevelTransitionTimerHandle);
        GetWorldTimerManager().ClearTimer(RaidCountdownTimerHandle);

        ProcessSettlement(false);
    }
}

void ABaruGameMode::AddTeamScrapValue(int32 ScrapValue)
{
    if (!CachedBaruGameState || ScrapValue <= 0) return;

    const int32 NewValue = CachedBaruGameState->GetTeamScrapValue() + ScrapValue;
    CachedBaruGameState->SetTeamScrapValue(NewValue);
}

void ABaruGameMode::ProcessSettlement(bool bAllExtracted)
{
    SetMatchPhase(EBaruMatchState::PostGame);

    const int32 TotalValue = CachedBaruGameState ? CachedBaruGameState->GetTeamScrapValue() : 0;
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Processing Settlement (Survived: %d, Total Team Value: %d)"), bAllExtracted, TotalValue);

    UBaruSaveGameSubsystem* SaveSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UBaruSaveGameSubsystem>() : nullptr;

    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        if (ABaruPlayerController* BaruPC = Cast<ABaruPlayerController>(Iterator->Get()))
        {
            if (!IsValid(BaruPC) || BaruPC->IsPendingKillPending()) continue;

            const ABaruPlayerState* PS = BaruPC->GetPlayerState<ABaruPlayerState>();
            const bool bPlayerSurvived = bAllExtracted && (PS && PS->IsAlive());
            const int32 EarnedGold = bPlayerSurvived ? TotalValue : FMath::RoundToInt(TotalValue * 0.1f);
            const FString PlayerName = PS ? PS->GetPlayerName() : TEXT("Operative");

            // 로컬 세이브 데이터에 정산 기록 반영
            if (SaveSubsystem)
            {
                SaveSubsystem->RecordRaidResult(PlayerName, EarnedGold, bPlayerSurvived);
            }

            // 클라이언트 UI 호출
            FBaruSettlementReport Report;
            Report.bSurvived = bPlayerSurvived;
            Report.AcquiredCurrency = EarnedGold;
            Report.ExtractedItemCount = bPlayerSurvived ? 5 : 0;

            BaruPC->Client_ShowSettlementUI(Report);
        }
    }
}