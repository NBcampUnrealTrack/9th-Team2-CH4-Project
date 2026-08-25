#pragma once

#include "CoreMinimal.h"
#include "BaruDatabaseTypes.generated.h"

// 정산 대상 수집 아이템 DTO
USTRUCT(BlueprintType)
struct FBaruSettlementItemData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BARU|Database")
    FString ItemId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BARU|Database")
    int32 Quantity = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BARU|Database")
    int32 Value = 0;
};

// 정산 요청 컨텍스트 DTO (스냅샷 전송용)
USTRUCT(BlueprintType)
struct FBaruSettlementContext
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BARU|Database")
    FString PlayerId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BARU|Database")
    bool bIsExtracted = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BARU|Database")
    int32 TotalEarnedGold = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BARU|Database")
    TArray<FBaruSettlementItemData> AcquiredLoots;
};

// 플레이어 영구 프로필 데이터 DTO
USTRUCT(BlueprintType)
struct FBaruPlayerProfileData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BARU|Database")
    FString PlayerId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BARU|Database")
    FString Nickname;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BARU|Database")
    int32 Gold = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BARU|Database")
    int32 Survivals = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BARU|Database")
    int32 Deaths = 0;
};

// 은닉처(Stash) 영구 보관 아이템 레코드 DTO[cite: 1, 2]
USTRUCT(BlueprintType)
struct FBaruStashItemRecord
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BARU|Database")
    FString ItemInstanceId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BARU|Database")
    FString PlayerId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BARU|Database")
    FString ItemId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BARU|Database")
    int32 Quantity = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BARU|Database")
    float Durability = 100.0f;
};

// 비동기 DB 작업 완료 통보 델리게이트
DECLARE_DELEGATE_TwoParams(FOnBaruSettlementCompleted, const FString& /*PlayerId*/, bool /*bSuccess*/);
DECLARE_DELEGATE_TwoParams(FOnBaruProfileLoaded, bool /*bSuccess*/, const FBaruPlayerProfileData& /*Profile*/);