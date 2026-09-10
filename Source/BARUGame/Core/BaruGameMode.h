#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/OnlineReplStructs.h"
#include "Core/BaruGameState.h"
#include "BaruGameMode.generated.h"

class ABaruPlayerController;
class ABaruPlayerState;
class ABaruCharacter;

struct FDisconnectedPlayerSnapshot
{
	FUniqueNetIdRepl UniqueId;
	TWeakObjectPtr<APawn> PreservedPawn;
	float DisconnectTime = 0.0f;
};

/**
 * 인게임(지하 던전 탐사) 전용 GameMode
 * - 레이드 제한 시간 타이머 및 타임오버 처리
 * - 전리품 가치 집계
 * - 사망자 관전 모드 전환 및 팀 전멸 검사
 * - 엘리베이터 탈출 구역 진입 집계 및 정산 처리
 */
UCLASS()
class BARUGAME_API ABaruGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABaruGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	//매치 단계 전이 (대기 -> 탐사 -> 탈출 -> 정산)
	UFUNCTION(BlueprintCallable, Category = "BARU|GameMode")
	void SetMatchPhase(EBaruMatchState NewPhase);

	// 탈출 구역 내 진입 인원 변경 시 호출
	UFUNCTION(BlueprintCallable, Category = "BARU|GameMode")
	void OnExtractionZoneCountChanged(int32 InZoneCount);

	// 탈출 엘리베이터 작동 시 시네마틱 연출 후 레벨 전환 요청
	UFUNCTION(BlueprintCallable, Category = "BARU|GameMode")
	void RequestLevelTransition(const FString& TargetMapURL);

	// 플레이어가 스크랩 아이템을 수집/입금했을 때 가치 누적
	UFUNCTION(BlueprintCallable, Category = "BARU|GameMode")
	void AddTeamScrapValue(int32 ScrapValue);

	// 플레이어 사망 감지 핸들러
	UFUNCTION(BlueprintCallable, Category = "BARU|GameMode")
	void OnPlayerDied(AController* VictimController, AActor* KillerActor);

	// 최종 보상 정산 및 SaveGame 영구 기록 실행
	UFUNCTION(BlueprintCallable, Category = "BARU|GameMode")
	void ProcessSettlement(bool bAllExtracted);
	
	// 몬스터 사망 시 개인 킬 카운트 누적
	UFUNCTION(BlueprintCallable, Category = "BARU|GameMode")
	void OnMonsterDied(AActor* MonsterActor, AActor* KillerActor);

protected:
	void CheckTeamWipe();
	virtual void UpdateAlivePlayerCount();

	void StartRaidTimer();
	void UpdateRaidCountdown();
	void OnRaidTimeout();

	void StartSpectating(APlayerController* DeadController);
	void ExecuteServerTravel();
	
	void CleanUpExpiredSnapshots();

protected:
	UPROPERTY(Transient)
	TObjectPtr<ABaruGameState> CachedBaruGameState;

	// 탐사 제한 시간 (초 단위)
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Rules")
	int32 RaidDurationInSeconds = 900;

	FTimerHandle RaidCountdownTimerHandle;

	// 레벨 전환 연출 대기용 타이머 및 목적지 URL
	FTimerHandle LevelTransitionTimerHandle;
	FString PendingTargetMapURL;
	
	// 시네마틱 재생 후 실제 이동까지의 대기 시간
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Rules")
	float TransitionDelayDuration = 3.5f;
	
	// 탐사 실패/시간 초과 정산 후 복귀할 기본 로비 맵 (에디터 디폴트 설정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Rules")
	FString DefaultReturnMapURL = TEXT("/Game/BARUGame/Maps/Company01_Lobby");

	// 정산 UI 확인 후 로비로 강제 이동하기까지의 대기 시간 (초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Rules")
	float PostSettlementReturnDelay = 8.0f;

	FTimerHandle PostSettlementTimerHandle;
	
protected:
	// 접속 해제자 스냅샷 맵
	TMap<FString, FDisconnectedPlayerSnapshot> DisconnectedSnapshots;

	// 접속이 끊긴 경우 90초의 유예 시간을 부여
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Rules")
	float ReconnectGracePeriod = 90.0f;
	
	// 사망 직후 일정 연출 시간 필요 (5초)
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Rules")
	float DeathSpectateDelay = 5.0f;
};