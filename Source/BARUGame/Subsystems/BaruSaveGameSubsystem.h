#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Save/BaruSaveGame.h"
#include "BaruSaveGameSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBaruSaveCompleted, const FString&, SlotName, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBaruLoadCompleted, const FString&, SlotName, bool, bSuccess);

/**
 * 세이브 파일 입출력 및 메모리 캐싱을 총괄하는 GameInstance 서브시스템 ( AES-256 암호화 및 SHA-1 무결성 검증 적용 )
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

	// 현재 캐시된 활성 슬롯 세이브 데이터 비동기 저장
	UFUNCTION(BlueprintCallable, Category = "BARU|SaveGame")
	void SaveCurrentGameAsync();

	// 특정 슬롯 비동기 저장
	UFUNCTION(BlueprintCallable, Category = "BARU|SaveGame")
	void SaveGameBySlotAsync(const FString& InSlotName);
	
	// 동기식 슬롯 저장 (에디터 종료/Deinitialize 전용)
	UFUNCTION(BlueprintCallable, Category = "BARU|SaveGame")
	bool SaveGameBySlot(const FString& InSlotName);
	
	// 레이드 정산 결과 반영 및 즉시 저장
	UFUNCTION(BlueprintCallable, Category = "BARU|SaveGame")
	void RecordRaidResult(const FString& InPlayerName, int32 EarnedGold, bool bSurvived);

	// 현재 메모리에 캐시된 활성 슬롯의 세이브 객체 반환
	UFUNCTION(BlueprintPure, Category = "BARU|SaveGame")
	UBaruSaveGame* GetCachedSaveGame() const;
	
	// 특정 플레이어 슬롯의 캐시 세이브 객체 반환
	UFUNCTION(BlueprintPure, Category = "BARU|SaveGame")
	UBaruSaveGame* GetCachedSaveGameByPlayer(const FString& InPlayerName) const;

public:
	UPROPERTY(BlueprintAssignable, Category = "BARU|SaveGame|Delegates")
	FOnBaruSaveCompleted OnSaveCompletedEvent;

	UPROPERTY(BlueprintAssignable, Category = "BARU|SaveGame|Delegates")
	FOnBaruLoadCompleted OnLoadCompletedEvent;

protected:
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UBaruSaveGame>> CachedSaveGames;

	FString CurrentSlotName;
	const int32 UserIndex = 0;
	
private:
	// 슬롯 이름을 디스크의 실제 .sav 파일 절대 경로로 변환
	FString GetSaveFilePath(const FString& SlotName) const;

	// 동기식 암호화 저장 내부 로직 (메모리 직렬화 -> AES-256 -> SHA-256 -> 파일 저장)
	bool SaveEncryptedSlotInternal(UBaruSaveGame* SaveObject, const FString& SlotName);

	// 동기식 복호화 로드 내부 로직 (파일 읽기 -> SHA-256 검증 -> 복호화 -> 메모리 역직렬화)
	UBaruSaveGame* LoadEncryptedSlotInternal(const FString& SlotName);

	// 암호화된 세이브 파일이 디스크에 유효하게 존재하는지 확인
	bool DoesEncryptedSaveExist(const FString& SlotName) const;
};