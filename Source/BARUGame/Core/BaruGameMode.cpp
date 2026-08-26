#include "Core/BaruGameMode.h"
#include "Core/BaruGameState.h"
#include "Player/BaruPlayerController.h"
#include "Player/BaruPlayerState.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "BaruLog.h"

ABaruGameMode::ABaruGameMode()
{
    PrimaryActorTick.bCanEverTick = false;

    GameStateClass = ABaruGameState::StaticClass();
    PlayerControllerClass = ABaruPlayerController::StaticClass();
    PlayerStateClass = ABaruPlayerState::StaticClass();
}

void ABaruGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    BARU_LOG(LogBaruSession, Log, TEXT("ABaruGameMode::InitGame on Map: %s"), *MapName);
}

void ABaruGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (IsValid(NewPlayer))
    {
        BARU_NET_LOG(NewPlayer, LogBaruSession, Log, TEXT("Player Logged In: %s"), *NewPlayer->GetName());
    }

    UpdateAlivePlayerCount();
}

void ABaruGameMode::Logout(AController* Exiting)
{
    if (IsValid(Exiting))
    {
        BARU_NET_LOG(Exiting, LogBaruSession, Log, TEXT("Player Logged Out: %s"), *Exiting->GetName());
        
        if (APawn* ControlledPawn = Exiting->GetPawn())
        {
            ControlledPawn->Destroy();
        }
    }

    Super::Logout(Exiting);

    UpdateAlivePlayerCount();
    CheckTeamWipe();
}

void ABaruGameMode::UpdateAlivePlayerCount()
{
    if (!CachedBaruGameState)
    {
        CachedBaruGameState = GetGameState<ABaruGameState>();
    }

    if (!CachedBaruGameState)
    {
        return;
    }
    
    int32 CurrentAlive = 0;
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        APlayerController* PC = Iterator->Get();
        if (IsValid(PC) && !PC->IsPendingKillPending())
        {
            if (const ABaruPlayerState* PS = PC->GetPlayerState<ABaruPlayerState>())
            {
                // TODO : 생존 관련 정의를 명확히 하고 생존자 카운트 조건 수정 필요
                if (!PS->IsDBNO())
                {
                    CurrentAlive++;
                }
            }
        }
    }
    
    CachedBaruGameState->SetAlivePlayerCount(CurrentAlive);
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Alive Player Count Updated: %d"), CurrentAlive);
}

// Level Transition

void ABaruGameMode::RequestLevelTransition(const FString& TargetMapURL)
{
    if (TargetMapURL.IsEmpty())
    {
        BARU_LOG(LogBaruSession, Warning, TEXT("RequestLevelTransition Rejected: Empty Map URL."));
        return;
    }
    
    if (GetWorldTimerManager().IsTimerActive(LevelTransitionTimerHandle))
    {
        BARU_LOG(LogBaruSession, Warning, TEXT("RequestLevelTransition Ignored: Transition already in progress."));
        return;
    }

    PendingTargetMapURL = TargetMapURL;
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Transition Requested to: %s. Broadcasting Cinematic RPC..."), *TargetMapURL);
    
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
    
    // Todo : 레벨 기믹 구현시 해당 ServerTravel() 함수 호출하여 실제 레벨 이동
    GetWorld()->ServerTravel(PendingTargetMapURL + TEXT("?listen"));
}


// 탐사 및 정산 로직

void ABaruGameMode::AddTeamScrapValue(int32 ScrapValue)
{
    if (!CachedBaruGameState || ScrapValue <= 0)
    {
        return;
    }

    const int32 NewValue = CachedBaruGameState->GetTeamScrapValue() + ScrapValue;
    CachedBaruGameState->SetTeamScrapValue(NewValue);
}

void ABaruGameMode::OnPlayerDied(AController* VictimController, AActor* KillerActor)
{
    BARU_NET_LOG(VictimController, LogBaruCombat, Log, TEXT("Player Died: %s by Killer: %s"), 
        VictimController ? *VictimController->GetName() : TEXT("None"), 
        KillerActor ? *KillerActor->GetName() : TEXT("None"));

    UpdateAlivePlayerCount();
    CheckTeamWipe();
}

void ABaruGameMode::CheckTeamWipe()
{
    if (CachedBaruGameState && CachedBaruGameState->GetAlivePlayerCount() <= 0)
    {
        BARU_NET_LOG(this, LogBaruSession, Warning, TEXT("Team wiped. Canceling ongoing transition and Processing Settlement."));
        
        GetWorldTimerManager().ClearTimer(LevelTransitionTimerHandle);

        ProcessSettlement(false);
    }
}

void ABaruGameMode::ProcessSettlement(bool bAllExtracted)
{
    const int32 TotalValue = CachedBaruGameState ? CachedBaruGameState->GetTeamScrapValue() : 0;
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Processing Settlement (Survived: %d, Total Team Value: %d)"), bAllExtracted, TotalValue);

    // TODO: [백엔드] UBaruDatabaseSubsystem을 통한 영구 DB 저장 트랜잭션 호출
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        if (ABaruPlayerController* BaruPC = Cast<ABaruPlayerController>(Iterator->Get()))
        {
            FBaruSettlementReport Report;
            Report.bSurvived = bAllExtracted;
            Report.AcquiredCurrency = bAllExtracted ? TotalValue : FMath::RoundToInt(TotalValue * 0.1f);
            
            // Todo : PS->InventoryComponent->GetTotalItemCount() 형태로 교체
            Report.ExtractedItemCount = bAllExtracted ? 5 : 0;

            BaruPC->Client_ShowSettlementUI(Report);
        }
    }
}