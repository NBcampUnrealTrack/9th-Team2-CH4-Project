#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BaruTestGameMode.generated.h"

class ABaruGameState;
class ABaruPlayerController;
class ABaruPlayerState;
class ABaruCharacter;

/**
 * 멀티플레이 네트워크 동기화, GAS, 폰 스폰 및 리스폰 검증 전용 경량 게임모드
 */
UCLASS()
class BARUGAME_API ABaruTestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABaruTestGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	// 테스트 편의 기능: 사망 시 즉시 또는 딜레이 후 리스폰
	UFUNCTION(BlueprintCallable, Category = "BARU|Test")
	void OnPlayerDied(AController* VictimController, AActor* KillerActor);

	UFUNCTION(BlueprintCallable, Category = "BARU|Test")
	void RespawnPlayer(AController* TargetController);

protected:
	void UpdateTestPlayerCount();

protected:
	UPROPERTY(Transient)
	TObjectPtr<ABaruGameState> CachedBaruGameState;

	// 사망 후 자동 리스폰 딜레이 (초 단위, 0이면 즉시)
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Test")
	float AutoRespawnDelay = 2.0f;
};