#include "Core/BaruGameMode.h"
#include "Core/BaruGameState.h"
#include "Player/BaruPlayerController.h"
#include "Player/BaruPlayerState.h"
#include "Engine/World.h"
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

    BARU_NET_LOG(NewPlayer, LogBaruSession, Log, TEXT("Player Logged In: %s"), *NewPlayer->GetName());

    if (!CachedBaruGameState)
    {
        CachedBaruGameState = GetGameState<ABaruGameState>();
    }

    if (CachedBaruGameState)
    {
        CachedBaruGameState->SetAlivePlayerCount(GetNumPlayers());
    }
}

void ABaruGameMode::Logout(AController* Exiting)
{
    BARU_NET_LOG(Exiting, LogBaruSession, Log, TEXT("Player Logged Out: %s"), *Exiting->GetName());

    Super::Logout(Exiting);

    if (CachedBaruGameState)
    {
        CachedBaruGameState->SetAlivePlayerCount(GetNumPlayers());
        CheckTeamWipe();
    }
}

// Level Transition

void ABaruGameMode::RequestLevelTransition(const FString& TargetMapURL)
{
    if (TargetMapURL.IsEmpty())
    {
        BARU_LOG(LogBaruSession, Warning, TEXT("RequestLevelTransition Rejected: Empty Map URL."));
        return;
    }

    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Executing ServerTravel to: %s"), *TargetMapURL);
    
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        if (ABaruPlayerController* BaruPC = Cast<ABaruPlayerController>(Iterator->Get()))
        {
            BaruPC->Client_PlayElevatorCinematic();
        }
    }

    // TODO: [레벨 기믹] 엘리베이터 액터의 카운트다운/도어 연출 완료 후 본 함수 호출
    GetWorld()->ServerTravel(TargetMapURL + TEXT("?listen"));
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

    if (CachedBaruGameState)
    {
        int32 CurrentAlive = 0;
        for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
        {
            if (const ABaruPlayerState* PS = Iterator->Get()->GetPlayerState<ABaruPlayerState>())
            {
                if (!PS->IsDBNO())
                {
                    CurrentAlive++;
                }
            }
        }
        CachedBaruGameState->SetAlivePlayerCount(CurrentAlive);
    }

    CheckTeamWipe();
}

void ABaruGameMode::CheckTeamWipe()
{
    if (CachedBaruGameState && CachedBaruGameState->GetAlivePlayerCount() <= 0)
    {
        BARU_NET_LOG(this, LogBaruSession, Warning, TEXT("Team wiped. Processing Failure Settlement."));
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