#include "Subsystems/BaruSaveGameSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "BaruLog.h"

UBaruSaveGameSubsystem::UBaruSaveGameSubsystem()
{
}

void UBaruSaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    BARU_LOG(LogBaruSession, Log, TEXT("BaruSaveGameSubsystem Initialized."));
}

void UBaruSaveGameSubsystem::Deinitialize()
{
    SaveCurrentGame();
    CachedSaveGame = nullptr;
    Super::Deinitialize();
}

UBaruSaveGame* UBaruSaveGameSubsystem::LoadOrCreateSaveGame(const FString& InPlayerName)
{
    CurrentSlotName = UBaruSaveGame::GetSlotName(InPlayerName);

    if (UGameplayStatics::DoesSaveGameExist(CurrentSlotName, UserIndex))
    {
        CachedSaveGame = Cast<UBaruSaveGame>(UGameplayStatics::LoadGameFromSlot(CurrentSlotName, UserIndex));
        BARU_LOG(LogBaruSession, Log, TEXT("SaveGame Loaded from slot: %s"), *CurrentSlotName);
    }

    if (!CachedSaveGame)
    {
        CachedSaveGame = Cast<UBaruSaveGame>(UGameplayStatics::CreateSaveGameObject(UBaruSaveGame::StaticClass()));
        if (CachedSaveGame)
        {
            CachedSaveGame->PlayerName = InPlayerName.IsEmpty() ? TEXT("Operative") : InPlayerName;
        }
        BARU_LOG(LogBaruSession, Log, TEXT("New SaveGame Created for: %s"), *InPlayerName);
    }

    OnLoadCompletedEvent.Broadcast(CurrentSlotName, CachedSaveGame != nullptr);
    return CachedSaveGame;
}

bool UBaruSaveGameSubsystem::SaveCurrentGame()
{
    if (!CachedSaveGame || CurrentSlotName.IsEmpty())
    {
        BARU_LOG(LogBaruSession, Warning, TEXT("SaveCurrentGame Failed: No cached save game or invalid slot name."));
        return false;
    }

    const bool bSuccess = UGameplayStatics::SaveGameToSlot(CachedSaveGame, CurrentSlotName, UserIndex);
    BARU_LOG(LogBaruSession, Log, TEXT("SaveGame to slot '%s': %s"), *CurrentSlotName, bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));
    
    OnSaveCompletedEvent.Broadcast(CurrentSlotName, bSuccess);
    return bSuccess;
}

void UBaruSaveGameSubsystem::RecordRaidResult(const FString& InPlayerName, int32 EarnedGold, bool bSurvived)
{
    UBaruSaveGame* SaveData = LoadOrCreateSaveGame(InPlayerName);
    if (!SaveData)
    {
        return;
    }

    SaveData->TotalGold += EarnedGold;
    if (bSurvived)
    {
        SaveData->TotalSurvivals++;
    }
    else
    {
        SaveData->TotalDeaths++;
    }

    SaveCurrentGame();
}