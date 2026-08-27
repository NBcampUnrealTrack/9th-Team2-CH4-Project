#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Save/BaruSaveGame.h"
#include "BaruSaveGameSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBaruSaveCompleted, const FString&, SlotName, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBaruLoadCompleted, const FString&, SlotName, bool, bSuccess);

/**
 * 세이브 파일 입출력 및 메모리 캐싱을 총괄하는 GameInstance 서브시스템
 */
UCLASS()
class BARUGAME_API UBaruSaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UBaruSaveGameSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 세이브 데이터 로드 또는 신규 생성
	UFUNCTION(BlueprintCallable, Category = "BARU|SaveGame")
	UBaruSaveGame* LoadOrCreateSaveGame(const FString& InPlayerName);

	// 현재 캐시된 세이브 데이터 디스크 저장
	UFUNCTION(BlueprintCallable, Category = "BARU|SaveGame")
	bool SaveCurrentGame();

	// 레이드 정산 결과 반영 및 즉시 저장
	UFUNCTION(BlueprintCallable, Category = "BARU|SaveGame")
	void RecordRaidResult(const FString& InPlayerName, int32 EarnedGold, bool bSurvived);

	// 현재 메모리에 캐시된 세이브 객체 반환
	UFUNCTION(BlueprintPure, Category = "BARU|SaveGame")
	UBaruSaveGame* GetCachedSaveGame() const { return CachedSaveGame; }

public:
	UPROPERTY(BlueprintAssignable, Category = "BARU|SaveGame|Delegates")
	FOnBaruSaveCompleted OnSaveCompletedEvent;

	UPROPERTY(BlueprintAssignable, Category = "BARU|SaveGame|Delegates")
	FOnBaruLoadCompleted OnLoadCompletedEvent;

protected:
	UPROPERTY(Transient)
	TObjectPtr<UBaruSaveGame> CachedSaveGame;

	FString CurrentSlotName;
	const int32 UserIndex = 0;
};