#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/EngineTypes.h"
#include "Core/BaruGameState.h"
#include "BaruGameMode.generated.h"

class ABaruPlayerController;
class ABaruPlayerState;
class ABaruCharacter;

/**
 * 세션 수명 주기, 레벨 전환, 생존자 집계 및 정산 판정 총괄
 */
UCLASS()
class BARUGAME_API ABaruGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABaruGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	
	virtual void BeginPlay() override;

	// Match
	UFUNCTION(BlueprintCallable, Category = "BARU|GameMode")
	void SetMatchPhase(EBaruMatchState NewPhase);
	
	// Exit (Gimmick)
	UFUNCTION(BlueprintCallable, Category = "BARU|GameMode")
	void OnExtractionZoneCountChanged(int32 InZoneCount);
	
	// Level Transition 위임 함수
	UFUNCTION(BlueprintCallable, Category = "BARU|GameMode")
	void RequestLevelTransition(const FString& TargetMapURL);

	// 탐사, 사망, 정산
	UFUNCTION(BlueprintCallable, Category = "BARU|GameMode")
	void AddTeamScrapValue(int32 ScrapValue);

	UFUNCTION(BlueprintCallable, Category = "BARU|GameMode")
	void OnPlayerDied(AController* VictimController, AActor* KillerActor);

	UFUNCTION(BlueprintCallable, Category = "BARU|GameMode")
	void ProcessSettlement(bool bAllExtracted);

protected:

	// 생존자 수 재집계 & 전멸 검사
	void CheckTeamWipe();
	virtual void UpdateAlivePlayerCount();
	
	// 탐사 제한 시간 타이머
	void StartRaidTimer();
	void UpdateRaidCountdown();
	void OnRaidTimeout();
	
	// 사망 플레이어 관전 모드
	void StartSpectating(APlayerController* DeadController);
	
	// 레벨 전환
	void ExecuteServerTravel();

protected:
	UPROPERTY(Transient)
	TObjectPtr<ABaruGameState> CachedBaruGameState;

	// 레이드 제한 시간 설정 (초 단위)
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Rules")
	int32 RaidDurationInSeconds = 900;
	
	FTimerHandle RaidCountdownTimerHandle;
	
	// 레벨 전환 연출 대기용 타이머 및 목적지 URL
	FTimerHandle LevelTransitionTimerHandle;
	FString PendingTargetMapURL;
	
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Rules")
	float TransitionDelayDuration = 3.5f;
	
	// TODO: [레벨 기믹] 엘리베이터 액터 구현 후 
	// 탑승 인원 체크 완료 시 GameMode->RequestLevelTransition() 또는 GameMode->ProcessRaidSettlement()를 호출하도록 연동
};