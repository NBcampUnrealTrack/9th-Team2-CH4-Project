#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "BaruGameState.generated.h"

// UI Delegate
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruAlivePlayerCountChanged, int32, NewAliveCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruTeamScrapValueChanged, int32, NewTeamValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBaruGlobalNotificationReceived, const FText&, MessageText, float, DisplayDuration);

/**
 * 생존자 수, 팀 수집물 총 가치등 GameState 및 전역 브로드캐스트(Notification) 관리
 * Todo : 전역 관리가 필요한 정보가 있다면 추가
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
	int32 GetAlivePlayerCount() const { return AlivePlayerCount; }

	UFUNCTION(BlueprintPure, Category = "BARU|GameState")
	int32 GetTeamScrapValue() const { return TeamScrapValue; }

	// [Server Only] Setter
	UFUNCTION(BlueprintAuthorityOnly, Category = "BARU|GameState")
	void SetAlivePlayerCount(int32 NewCount);

	UFUNCTION(BlueprintAuthorityOnly, Category = "BARU|GameState")
	void SetTeamScrapValue(int32 NewValue);

	// Multicast RPC
	UFUNCTION(NetMulticast, Reliable, Category = "BARU|GameState")
	void Multicast_BroadcastNotification(const FText& MessageText, float DisplayDuration = 3.0f);

public:
	// UI 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "BARU|GameState|Events")
	FOnBaruAlivePlayerCountChanged OnAlivePlayerCountChanged;

	UPROPERTY(BlueprintAssignable, Category = "BARU|GameState|Events")
	FOnBaruTeamScrapValueChanged OnTeamScrapValueChanged;

	UPROPERTY(BlueprintAssignable, Category = "BARU|GameState|Events")
	FOnBaruGlobalNotificationReceived OnGlobalNotificationReceived;
	
protected:
	// Replicated Properties & RepNotifies
	UPROPERTY(ReplicatedUsing = OnRep_AlivePlayerCount, VisibleInstanceOnly, Category = "BARU|State")
	int32 AlivePlayerCount = 0;

	UPROPERTY(ReplicatedUsing = OnRep_TeamScrapValue, VisibleInstanceOnly, Category = "BARU|State")
	int32 TeamScrapValue = 0;

	UFUNCTION()
	virtual void OnRep_AlivePlayerCount();

	UFUNCTION()
	virtual void OnRep_TeamScrapValue();
};
