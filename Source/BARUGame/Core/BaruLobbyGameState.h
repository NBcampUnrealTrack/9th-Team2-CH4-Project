#pragma once

#include "CoreMinimal.h"
#include "Core/BaruGameState.h"
#include "BaruLobbyGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruAllPlayersReadyChanged, bool, bAllReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruTargetMapChanged, const FString&, NewMapURL);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBaruLobbyPlayerArrayUpdated);

class APlayerState;

UCLASS()
class BARUGAME_API ABaruLobbyGameState : public ABaruGameState
{
	GENERATED_BODY()

public:
	ABaruLobbyGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	// =========================================================================
	// Getters [Client & Host]
	// =========================================================================
	UFUNCTION(BlueprintPure, Category = "BARU|Lobby|GameState")
	bool IsAllPlayersReady() const { return bAllPlayersReady; }

	UFUNCTION(BlueprintPure, Category = "BARU|Lobby|GameState")
	FString GetSelectedTargetMapURL() const { return SelectedTargetMapURL; }

	// =========================================================================
	// Setters [Server Only]
	// =========================================================================
	UFUNCTION(BlueprintAuthorityOnly, Category = "BARU|Lobby|GameState")
	void SetAllPlayersReady(bool bInAllReady);

	UFUNCTION(BlueprintAuthorityOnly, Category = "BARU|Lobby|GameState")
	void SetSelectedTargetMapURL(const FString& InMapURL);

public:
	// UI 이벤트 바인딩용 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "BARU|Lobby|GameState|Events")
	FOnBaruAllPlayersReadyChanged OnAllPlayersReadyChanged;

	UPROPERTY(BlueprintAssignable, Category = "BARU|Lobby|GameState|Events")
	FOnBaruTargetMapChanged OnTargetMapChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "BARU|Lobby|GameState|Events")
	FOnBaruLobbyPlayerArrayUpdated OnLobbyPlayerArrayUpdated;

protected:
	// 전원 레디 상태
	UPROPERTY(ReplicatedUsing = OnRep_AllPlayersReady, VisibleInstanceOnly, Category = "BARU|Lobby|State")
	bool bAllPlayersReady = false;

	// 선택된 탐사 목적지 맵 URL
	UPROPERTY(ReplicatedUsing = OnRep_SelectedTargetMapURL, VisibleInstanceOnly, Category = "BARU|Lobby|State")
	FString SelectedTargetMapURL;

	UFUNCTION()
	virtual void OnRep_AllPlayersReady();

	UFUNCTION()
	virtual void OnRep_SelectedTargetMapURL();
};