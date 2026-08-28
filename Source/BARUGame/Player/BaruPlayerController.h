#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BaruPlayerController.generated.h"

// ★[삭제] class UInputMappingContext;
// ★[삭제] class UInputAction;
//   입력 관련 프로퍼티를 전부 ABaruCharacter 로 일원화했습니다(아래 protected 참고).

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

	// ★[추가] GAS 태그 기반 입력 처리 지점.
	//   UBaruAbilitySystemComponent 에 AbilityInputTagPressed / AbilityInputTagReleased /
	//   ProcessAbilityInput 이 전부 구현되어 있는데 프로젝트 전체에서 호출하는 곳이 0개였습니다.
	//   ProcessAbilityInput 이 안 돌면 눌림/뗌 큐(InputPressedSpecHandles)가 비워지지 않아
	//   어빌리티가 영영 발동되지 않습니다.
	//   PostProcessInput 은 그 프레임 입력 처리가 끝난 직후 호출되는 훅이라 가장 적합합니다.
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	
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

	// ★[추가] ★★ 버그 수정의 핵심 ★★
	//   기존 Server_SendPing 은 서버에서 각 PC 의 OnPingReceived 를 Broadcast 했는데,
	//   델리게이트는 복제되지 않으므로 "서버 프로세스 안의 PC 객체"에서만 터졌습니다.
	//   즉 클라이언트 UI 에는 핑이 단 한 번도 도착하지 않았습니다.
	//   FVector_NetQuantize 를 쓰는 이유: 핑 좌표에 소수점 정밀도가 불필요해 패킷을 줄입니다.
	UFUNCTION(Client, Reliable, Category = "BARU|Feedback")
	void Client_ReceivePing(FVector_NetQuantize PingLocation, EBaruPingType PingType);
	
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
	
	// ★[삭제] DefaultMappingContext / MoveAction / LookAction / JumpAction / InteractAction 5개 전부 삭제.
	//   이유 1: 4개의 InputAction 은 여기 선언만 되고 바인딩하는 코드가 전혀 없는 죽은 변수였습니다.
	//   이유 2: DefaultMappingContext 는 ABaruCharacter 에도 같은 이름으로 있어서
	//           IMC 가 우선순위 0으로 두 번 등록되고 있었습니다.
	//           지금 실제로 동작이 검증된 경로는 Character 쪽이므로 그쪽을 남기고 여기를 지웁니다.
	//   ※ BP_BaruPlayerController 에 이 값들을 넣어두셨다면 그 설정은 사라집니다(무해).

	// ★[추가] 핑 최대 사거리. 기존에는 10000.0f 가 cpp 에 박혀 있었습니다.
	//   README 4-3장 "C++ 하드코딩 금지, 수치는 데이터로 분리" 대응.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Ping")
	float MaxPingDistance = 10000.0f;

	// ★[추가] 인벤토리 슬롯 상한. 기존 _Validate 의 매직넘버 100 을 대체.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Inventory")
	int32 MaxInventorySlotIndex = 100;
};