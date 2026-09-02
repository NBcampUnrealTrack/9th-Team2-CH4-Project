#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "BaruGameState.generated.h"

// Match Phase (대기 - 탐사 - 탈출 중 - 정산)
UENUM(BlueprintType)
enum class EBaruMatchState : uint8
{
	WaitingToStart  UMETA(DisplayName = "Waiting To Start"),
	InProgress      UMETA(DisplayName = "In Progress"),
	Extraction      UMETA(DisplayName = "Extraction"),
	PostGame        UMETA(DisplayName = "Post Game")
};

// [Ping] 타입 정의

UENUM(BlueprintType)
enum class EBaruPingType : uint8
{
	Move        UMETA(DisplayName = "Move"),
	Interact    UMETA(DisplayName = "Interact"),
	Stop        UMETA(DisplayName = "Stop"),
	Attack      UMETA(DisplayName = "Attack"),
	Danger      UMETA(DisplayName = "Danger")
};

// UI Delegate
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruMatchStateChanged, EBaruMatchState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruRaidTimerUpdated, int32, RemainingSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruAlivePlayerCountChanged, int32, NewAliveCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruTeamScrapValueChanged, int32, NewTeamValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBaruExtractionPlayerCountChanged, int32, CurrentInZone, int32, RequiredCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBaruPingReceived, FVector, PingLocation, EBaruPingType, PingType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBaruGlobalNotificationReceived, const FText&, MessageText, float, DisplayDuration);

/**
 * 생존자 수, 팀 수집물 총 가치등 GameState 및 전역 브로드캐스트(Notification) 관리
 */

UCLASS()
class BARUGAME_API ABaruGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	ABaruGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Getter
	UFUNCTION(BlueprintPure, Category = "BARU|GameState")
	EBaruMatchState GetMatchState() const { return MatchState; }

	UFUNCTION(BlueprintPure, Category = "BARU|GameState")
	int32 GetRemainingRaidTime() const { return RemainingRaidTime; }

	UFUNCTION(BlueprintPure, Category = "BARU|GameState")
	int32 GetAlivePlayerCount() const { return AlivePlayerCount; }

	UFUNCTION(BlueprintPure, Category = "BARU|GameState")
	int32 GetTeamScrapValue() const { return TeamScrapValue; }

	UFUNCTION(BlueprintPure, Category = "BARU|GameState")
	int32 GetPlayersInExtractionZoneCount() const { return PlayersInExtractionZoneCount; }

	// [Server Only] Setter
	UFUNCTION(BlueprintAuthorityOnly, Category = "BARU|GameState")
	void SetMatchState(EBaruMatchState NewState);

	UFUNCTION(BlueprintAuthorityOnly, Category = "BARU|GameState")
	void SetRemainingRaidTime(int32 NewTime);

	UFUNCTION(BlueprintAuthorityOnly, Category = "BARU|GameState")
	void SetAlivePlayerCount(int32 NewCount);

	UFUNCTION(BlueprintAuthorityOnly, Category = "BARU|GameState")
	void SetTeamScrapValue(int32 NewValue);

	UFUNCTION(BlueprintAuthorityOnly, Category = "BARU|GameState")
	void SetPlayersInExtractionZoneCount(int32 NewCount);

	// Multicast RPC
	UFUNCTION(NetMulticast, Reliable, Category = "BARU|GameState")
	void Multicast_BroadcastNotification(const FText& MessageText, float DisplayDuration = 3.0f);
	
	UFUNCTION(NetMulticast, Reliable, Category = "BARU|GameState")
	void Multicast_BroadcastPing(FVector PingLocation, EBaruPingType PingType);

public:
	// UI 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "BARU|GameState|Events")
	FOnBaruMatchStateChanged OnMatchStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "BARU|GameState|Events")
	FOnBaruRaidTimerUpdated OnRaidTimerUpdated;

	UPROPERTY(BlueprintAssignable, Category = "BARU|GameState|Events")
	FOnBaruAlivePlayerCountChanged OnAlivePlayerCountChanged;

	UPROPERTY(BlueprintAssignable, Category = "BARU|GameState|Events")
	FOnBaruTeamScrapValueChanged OnTeamScrapValueChanged;

	UPROPERTY(BlueprintAssignable, Category = "BARU|GameState|Events")
	FOnBaruExtractionPlayerCountChanged OnExtractionPlayerCountChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "BARU|GameState|Events")
	FOnBaruPingReceived OnPingReceived;
	
	UPROPERTY(BlueprintAssignable, Category = "BARU|GameState|Events")
	FOnBaruGlobalNotificationReceived OnGlobalNotificationReceived;
	
protected:
	// Replicated Properties & RepNotifies
	UPROPERTY(ReplicatedUsing = OnRep_MatchState, VisibleInstanceOnly, Category = "BARU|State")
	EBaruMatchState MatchState = EBaruMatchState::WaitingToStart;

	UPROPERTY(ReplicatedUsing = OnRep_RemainingRaidTime, VisibleInstanceOnly, Category = "BARU|State")
	int32 RemainingRaidTime = 0;

	UPROPERTY(ReplicatedUsing = OnRep_AlivePlayerCount, VisibleInstanceOnly, Category = "BARU|State")
	int32 AlivePlayerCount = 0;

	UPROPERTY(ReplicatedUsing = OnRep_TeamScrapValue, VisibleInstanceOnly, Category = "BARU|State")
	int32 TeamScrapValue = 0;

	UPROPERTY(ReplicatedUsing = OnRep_PlayersInExtractionZoneCount, VisibleInstanceOnly, Category = "BARU|State")
	int32 PlayersInExtractionZoneCount = 0;

	
	// RepNotifies
	UFUNCTION() virtual void OnRep_MatchState();
	UFUNCTION() virtual void OnRep_RemainingRaidTime();
	UFUNCTION() virtual void OnRep_AlivePlayerCount();
	UFUNCTION() virtual void OnRep_TeamScrapValue();
	UFUNCTION() virtual void OnRep_PlayersInExtractionZoneCount();
};
