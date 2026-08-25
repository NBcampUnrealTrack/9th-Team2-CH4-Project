#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SQLiteDatabase.h"
#include "Subsystems/BaruDatabaseTypes.h"
#include "BaruDatabaseSubsystem.generated.h"

/**
 * Dedicated Server 및 로컬 환경에서 SQLite 임베디드 DB 트랜잭션을 전담하는 서브시스템.
 * 모든 디스크 I/O는 백그라운드 워커 스레드에서 비동기(Async)로 수행되어 게임 스레드 히치를 방지
 */
UCLASS()
class BARUGAME_API UBaruDatabaseSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UBaruDatabaseSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 비동기 정산 트랜잭션 요청 (Dedicated Server GameMode 전용)
	void SaveSettlementAsync(const FBaruSettlementContext& Context, FOnBaruSettlementCompleted OnCompleted);

	// 비동기 프로필 데이터 로드 요청 (AGameModeBase::PostLogin 연동)
	void LoadPlayerProfileAsync(const FString& PlayerId, FOnBaruProfileLoaded OnCompleted);

private:
	FString DbPath;
	FCriticalSection DbLock; // 멀티스레드 DB 접근 동기화 락

	// 테이블 자동 생성 (Users, Stash_Items, Settlement_Logs)
	void InitializeTables();

	// 워커 스레드 내부 실행 함수
	bool ExecuteSettlementTransaction_Internal(const FBaruSettlementContext& Context);
	bool ExecuteLoadProfile_Internal(const FString& PlayerId, FBaruPlayerProfileData& OutProfile);
};