#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BaruPlayerController.generated.h"

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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBaruPingReceived, FVector, PingLocation, EBaruPingType, PingType);

/**
 * 클라이언트 입력, 서버 RPC 라우팅 및 1회성 피드백 담당 처리
 */

UCLASS()
class BARUGAME_API ABaruPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ABaruPlayerController();
	
	// Server RPC
	// Todo : 아이템 관련 RPC의 경우 InventoryComponent 또는 EquipmentComponent로 처리 위임해야 함
	UFUNCTION(Server, Reliable, WithValidation, Category = "BARU|Input")
	void Server_RequestEquipItem(int32 SlotIndex);

	UFUNCTION(Server, Reliable, WithValidation, Category = "BARU|Input")
	void Server_RequestUseItem(int32 SlotIndex);

	UFUNCTION(Server, Reliable, WithValidation, Category = "BARU|Input")
	void Server_RequestDropItem(int32 SlotIndex, int32 Count);

	// Todo : 현재 핑 시스템은 PlayerController를 순회하는 방식이나 팀/세션 단위 핑 시스템 확장시 GameState로 전송을 위임
	UFUNCTION(Server, Reliable, WithValidation, Category = "BARU|Input")
	void Server_SendPing(FVector PingLocation, EBaruPingType PingType);
	
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

	UPROPERTY(BlueprintAssignable, Category = "BARU|Events")
	FOnBaruPingReceived OnPingReceived;
	
protected:
	virtual void BeginPlay() override;
};
