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
    for (const auto& Pair : CachedSaveGames)
    {
        if (Pair.Value)
        {
            UGameplayStatics::SaveGameToSlot(Pair.Value, Pair.Key, UserIndex);
        }
    }
    CachedSaveGames.Empty();

    BARU_LOG(LogBaruSession, Log, TEXT("BaruSaveGameSubsystem Deinitialized."));
    Super::Deinitialize();
}

UBaruSaveGame* UBaruSaveGameSubsystem::LoadOrCreateSaveGame(const FString& InPlayerName)
{
    CurrentSlotName = UBaruSaveGame::GetSlotName(InPlayerName);

    if (TObjectPtr<UBaruSaveGame>* FoundSave = CachedSaveGames.Find(CurrentSlotName))
    {
        if (FoundSave && FoundSave->Get())
        {
            OnLoadCompletedEvent.Broadcast(CurrentSlotName, true);
            return FoundSave->Get();
        }
    }

    UBaruSaveGame* TargetSaveGame = nullptr;
    
    if (UGameplayStatics::DoesSaveGameExist(CurrentSlotName, UserIndex))
    {
        TargetSaveGame = Cast<UBaruSaveGame>(UGameplayStatics::LoadGameFromSlot(CurrentSlotName, UserIndex));
        BARU_LOG(LogBaruSession, Log, TEXT("SaveGame Loaded from slot: %s"), *CurrentSlotName);
    }

    if (!TargetSaveGame)
    {
        TargetSaveGame = Cast<UBaruSaveGame>(UGameplayStatics::CreateSaveGameObject(UBaruSaveGame::StaticClass()));
        if (TargetSaveGame)
        {
            TargetSaveGame->PlayerName = InPlayerName.IsEmpty() ? TEXT("Operative") : InPlayerName;
        }
        BARU_LOG(LogBaruSession, Log, TEXT("New SaveGame Created for: %s"), *InPlayerName);
    }
    
    if (TargetSaveGame)
    {
        CachedSaveGames.Add(CurrentSlotName, TargetSaveGame);
    }

    OnLoadCompletedEvent.Broadcast(CurrentSlotName, TargetSaveGame != nullptr);
    return TargetSaveGame;
}

bool UBaruSaveGameSubsystem::SaveCurrentGame()
{
    return SaveGameBySlot(CurrentSlotName);
}

bool UBaruSaveGameSubsystem::SaveGameBySlot(const FString& InSlotName)
{
    if (InSlotName.IsEmpty())
    {
        BARU_LOG(LogBaruSession, Warning, TEXT("SaveGame Failed: SlotName is empty."));
        return false;
    }

    TObjectPtr<UBaruSaveGame>* FoundSave = CachedSaveGames.Find(InSlotName);
    if (!FoundSave || !FoundSave->Get())
    {
        BARU_LOG(LogBaruSession, Warning, TEXT("SaveGame Failed: No cached save game for slot '%s'."), *InSlotName);
        return false;
    }

    const bool bSuccess = UGameplayStatics::SaveGameToSlot(FoundSave->Get(), InSlotName, UserIndex);
    BARU_LOG(LogBaruSession, Log, TEXT("SaveGame to slot '%s': %s"), *InSlotName, bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));

    OnSaveCompletedEvent.Broadcast(InSlotName, bSuccess);
    return bSuccess;
}

void UBaruSaveGameSubsystem::RecordRaidResult(const FString& InPlayerName, int32 EarnedGold, bool bSurvived)
{
    const FString SlotName = UBaruSaveGame::GetSlotName(InPlayerName);

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

    SaveGameBySlot(SlotName);
}

UBaruSaveGame* UBaruSaveGameSubsystem::GetCachedSaveGame() const
{
    if (const TObjectPtr<UBaruSaveGame>* FoundSave = CachedSaveGames.Find(CurrentSlotName))
    {
        return FoundSave->Get();
    }
    return nullptr;
}

UBaruSaveGame* UBaruSaveGameSubsystem::GetCachedSaveGameByPlayer(const FString& InPlayerName) const
{
    const FString SlotName = UBaruSaveGame::GetSlotName(InPlayerName);
    if (const TObjectPtr<UBaruSaveGame>* FoundSave = CachedSaveGames.Find(SlotName))
    {
        return FoundSave->Get();
    }
    return nullptr;
}