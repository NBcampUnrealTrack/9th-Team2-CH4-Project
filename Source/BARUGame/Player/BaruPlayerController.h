#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BaruPlayerController.generated.h"


class UInputAction;
class UInputMappingContext; // 다시 추가 관전 전환 입력은 PC에서 직접 바인딩해야하기 때문에 추가함
// [Result] 정산용 데이터 수신 구조체
// Todo : 별도의 DatabaseType으로 분리 예정
USTRUCT(BlueprintType)
struct FBaruSettlementReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Settlement")
	bool bSurvived = false;

	UPROPERTY(BlueprintReadOnly, Category = "Settlement")
	int32 AcquiredCurrency = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Settlement")
	int32 ExtractedItemCount = 0;
};

// UI Deligate (Client)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruSettlementReceived, const FBaruSettlementReport&, Report);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBaruPlayCinematic);

/**
 * 클라이언트 입력, 서버 RPC 라우팅 및 1회성 피드백 담당 처리
 */

UCLASS()
class BARUGAME_API ABaruPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ABaruPlayerController();

	
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	
	UFUNCTION(Server, Reliable, WithValidation, Category = "BARU|Spectate")
	void Server_CycleSpectatorTarget(bool bNext = true);
	
	// Server RPC
	// Todo : 아이템 관련 RPC의 경우 InventoryComponent 또는 EquipmentComponent로 처리 위임해야 함
	UFUNCTION(Server, Reliable, WithValidation, Category = "BARU|Input")
	void Server_RequestEquipItem(int32 SlotIndex);

	UFUNCTION(Server, Reliable, WithValidation, Category = "BARU|Input")
	void Server_RequestUseItem(int32 SlotIndex);

	UFUNCTION(Server, Reliable, WithValidation, Category = "BARU|Input")
	void Server_RequestDropItem(int32 SlotIndex, int32 Count);
	
	
	// [수정 09.08] BlueprintCallable 추가 — 로비 UI 위젯에서 직접 호출하기 위함
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "BARU|Lobby")
	void Server_RequestStartRaid();
	
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "BARU|Lobby")
	void Server_RequestSetTargetRaidMap(const FString& TargetMapURL);
	
	// Client RPC
	UFUNCTION(Client, Reliable, Category = "BARU|Feedback")
	void Client_ShowSettlementUI(const FBaruSettlementReport& Report);

	UFUNCTION(Client, Reliable, Category = "BARU|Feedback")
	void Client_PlayElevatorCinematic();

	
public:
	// UI 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "BARU|Events")
	FOnBaruSettlementReceived OnSettlementReceived;

	UPROPERTY(BlueprintAssignable, Category = "BARU|Events")
	FOnBaruPlayCinematic OnPlayCinematic;

	
protected:
	virtual void BeginPlay() override;
	
	virtual void SetupInputComponent() override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Spectate")
	TObjectPtr<UInputAction> SpectateNextAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Spectate")
	TObjectPtr<UInputAction> SpectatePrevAction;
	
	void Input_SpectateNext();
	void Input_SpectatePrev();
	
	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> CurrentSpectatingPawn;

	
	void GatherSpectatablePawns(TArray<APawn*>& OutPawns) const;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Lobby")
	TArray<FString> AllowedRaidMapURLs;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Ping")
	float MaxPingDistance = 10000.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Inventory")
	int32 MaxInventorySlotIndex = 100;
};