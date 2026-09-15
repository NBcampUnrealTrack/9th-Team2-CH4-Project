#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "BaruPlayerController.generated.h"


class UInputAction;
class UInputMappingContext; 
class UCommonActivatableWidget;   // [추가] 인벤토리 위젯
class UBaruItemFocusComponent;
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
	
	// 09.11 추가 요청(충돌시 같이 검토)
	UPROPERTY(BlueprintReadOnly, Category = "Settlement")
	int32 MonsterKillCount = 0;
};

// [추가] 홀드 상호작용(소생 등) 종료 이유 — UI 가 문구/연출 고를 때 사용
UENUM(BlueprintType)
enum class EBaruInteractionHoldEndReason : uint8
{
	Completed,	// 끝까지 채움
	Released,	// F 키를 뗌
	OutOfRange,	// 거리 이탈
	Failed,		// 대상 상태가 바뀜 (출혈사, 다른 사람이 먼저 소생 등)
	Cancelled,	// 내가 다운/사망, 다른 상호작용 시작 등
};
// UI Deligate (Client)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruSettlementReceived, const FBaruSettlementReport&, Report);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBaruPlayCinematic);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnBaruInteractionHoldStarted, AActor*, OtherActor, float, Duration, bool, bIsHolder);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnBaruInteractionHoldEnded, AActor*, OtherActor, EBaruInteractionHoldEndReason, Reason, bool, bIsHolder);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruSpectatorTargetChanged, const FString&, SpectatedPlayerName);

/**
 * 클라이언트 입력, 서버 RPC 라우팅 및 1회성 피드백 담당 처리
 */

UCLASS()
class BARUGAME_API ABaruPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ABaruPlayerController();
	
	// [추가] 심리스 트래블 완료 시 로컬 상태 초기화
	virtual void PostSeamlessTravel() override;
	
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	
	UFUNCTION(Server, Reliable, WithValidation, Category = "BARU|Spectate")
	void Server_CycleSpectatorTarget(bool bNext = true);
	
	void SetCurrentSpectatingPawn(APawn* InPawn) { CurrentSpectatingPawn = InPawn; }
	
	// Server RPC
	// Todo : 아이템 관련 RPC의 경우 InventoryComponent 또는 EquipmentComponent로 처리 위임해야 함
	UFUNCTION(Server, Reliable, WithValidation, Category = "BARU|Input")
	void Server_RequestEquipItem(int32 SlotIndex);

	UFUNCTION(Server, Reliable, WithValidation, Category = "BARU|Input")
	void Server_RequestUseItem(int32 SlotIndex);
	
	UFUNCTION(BlueprintCallable, Category = "BARU|UI")
	void ToggleInventory();
	
	// [09.13] ESC 키 게임 메뉴 토글 함수
	UFUNCTION(BlueprintCallable, Category = "BARU|UI")
	void ToggleGameMenu();

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
	
	UFUNCTION(Client, Reliable, Category = "BARU|Feedback")
	void Client_InteractionHoldStarted(AActor* OtherActor, float Duration, bool bIsHolder);

	UFUNCTION(Client, Reliable, Category = "BARU|Feedback")
	void Client_InteractionHoldEnded(AActor* OtherActor, EBaruInteractionHoldEndReason Reason, bool bIsHolder);
	
	UFUNCTION(BlueprintPure, Category = "BARU|Components")
	UBaruItemFocusComponent* GetItemFocusComponent() const { return ItemFocusComponent; }
	
	// [추가] 로비 복귀 시 클라이언트의 관전/대기 상태를 완전 해제하고 1인칭 조작/포커스를 복원하는 Client RPC
	UFUNCTION(Client, Reliable, Category = "BARU|Feedback")
	void Client_ResetLobbyInputAndState();
	
	UFUNCTION(Client, Reliable, Category = "BARU|Spectate")
	void Client_NotifySpectatingTargetChanged(const FString& SpectatedPlayerName);

public:
	// UI 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "BARU|Events")
	FOnBaruSettlementReceived OnSettlementReceived;

	UPROPERTY(BlueprintAssignable, Category = "BARU|Events")
	FOnBaruPlayCinematic OnPlayCinematic;
	
	UPROPERTY(BlueprintAssignable, Category = "BARU|Events")
	FOnBaruInteractionHoldStarted OnInteractionHoldStarted;
	
	UPROPERTY(BlueprintAssignable, Category = "BARU|Events")
	FOnBaruInteractionHoldEnded OnInteractionHoldEnded;
	
	UPROPERTY(BlueprintAssignable, Category = "BARU|Events")
	FOnBaruSpectatorTargetChanged OnSpectatorTargetChanged;
	
protected:
	virtual void BeginPlay() override;
	
	virtual void SetupInputComponent() override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Spectate")
	TObjectPtr<UInputAction> SpectateNextAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Spectate")
	TObjectPtr<UInputAction> SpectatePrevAction;
	
	void Input_SpectateNext();
	void Input_SpectatePrev();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
	TObjectPtr<UInputAction> ToggleInventoryAction;

	void Input_ToggleInventory();
	
	// [09.13] ESC 메뉴 오픈 인풋 액션 에셋 포인터
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
	TObjectPtr<UInputAction> MenuAction;

	void Input_ToggleGameMenu();
	
	// [09.13] ESC 눌렀을 때 띄울 위젯 클래스 (WBP_GameMenu)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|UI")
	TSubclassOf<UCommonActivatableWidget> GameMenuWidgetClass;
	
	// [09.13] 게임 메뉴가 닫힐 때(계속하기 클릭, CommonUI 뒤로가기 등) 호출될 콜백
	UFUNCTION()
	void HandleGameMenuDeactivated();

	// [09.13] 현재 열려 있는 게임 메뉴 위젯 참조
	UPROPERTY(Transient)
	TObjectPtr<UCommonActivatableWidget> ActiveGameMenuWidget;

	// BP_BaruPlayerController 에서 WBP_Inventory 를 지정합니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|UI")
	TSubclassOf<UCommonActivatableWidget> InventoryWidgetClass;

	// 현재 열려 있는 인벤토리 위젯. 열림/닫힘 판단에 씁니다.
	UPROPERTY(Transient)
	TObjectPtr<UCommonActivatableWidget> ActiveInventoryWidget;
	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> CurrentSpectatingPawn;

	
	void GatherSpectatablePawns(TArray<APawn*>& OutPawns) const;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Lobby")
	TArray<FString> AllowedRaidMapURLs;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Ping")
	float MaxPingDistance = 10000.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Inventory")
	int32 MaxInventorySlotIndex = 100;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
	TObjectPtr<UBaruItemFocusComponent> ItemFocusComponent;
	
	// [추가 09.14] 키 연타 및 더블 트리거 방지용 타임스탬프
	float LastInventoryToggleTime = 0.0f;
	float LastGameMenuToggleTime = 0.0f;
};