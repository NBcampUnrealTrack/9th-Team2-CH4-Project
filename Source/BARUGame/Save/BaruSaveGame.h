#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BaruSaveGame.generated.h"

/**
 * 디스크(.sav)에 직렬화(Serialize)되어 저장되는 순수 데이터 컨테이너
 */
UCLASS()
class BARUGAME_API UBaruSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, Category = "SaveData")
    int32 SaveVersion = 1;

    UPROPERTY(VisibleAnywhere, Category = "SaveData")
    FString PlayerName = TEXT("Operative");

    UPROPERTY(VisibleAnywhere, Category = "SaveData")
    int32 TotalGold = 0;

    UPROPERTY(VisibleAnywhere, Category = "SaveData")
    int32 TotalSurvivals = 0;

    UPROPERTY(VisibleAnywhere, Category = "SaveData")
    int32 TotalDeaths = 0;

    // 슬롯 이름 생성 헬퍼
    static FString GetSlotName(const FString& InPlayerName)
    {
        return InPlayerName.IsEmpty() ? TEXT("SaveSlot_Default") : FString::Printf(TEXT("SaveSlot_%s"), *InPlayerName);
    }
};