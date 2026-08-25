#include "Player/BaruPlayerController.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Player/BaruPlayerState.h"
#include "BaruLog.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"

ABaruPlayerController::ABaruPlayerController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABaruPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		BARU_LOG(LogBaruUI, Log, TEXT("Local PlayerController Initialized: %s"), *GetName());
	}
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        // 블루프린트에서 DefaultMappingContext를 잘 넣어뒀는지 확인 후 적용 (우선순위 0)
        if (DefaultMappingContext)
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
	// TODO : 멀티플레이에서 패킷 전송시 UI 인식이 안되는 것을 예방하는 방지 코드
}

// Server RPC
bool ABaruPlayerController::Server_RequestEquipItem_Validate(int32 SlotIndex)
{
    return SlotIndex >= 0 && SlotIndex < 100;
}

void ABaruPlayerController::Server_RequestEquipItem_Implementation(int32 SlotIndex)
{
    APawn* ControlledPawn = GetPawn();
    if (!IsValid(ControlledPawn))
    {
        BARU_NET_LOG(this, LogBaruItem, Warning, TEXT("Server_RequestEquipItem Rejected: Invalid Pawn."));
        return;
    }

    BARU_NET_LOG(this, LogBaruItem, Log, TEXT("Server_RequestEquipItem Approved for Slot: %d"), SlotIndex);
    // TODO: InventoryComponent / EquipmentComponent 장착 승인 로직 실행
}

bool ABaruPlayerController::Server_RequestUseItem_Validate(int32 SlotIndex)
{
    return SlotIndex >= 0 && SlotIndex < 100;
}

void ABaruPlayerController::Server_RequestUseItem_Implementation(int32 SlotIndex)
{
    APawn* ControlledPawn = GetPawn();
    if (!IsValid(ControlledPawn))
    {
        BARU_NET_LOG(this, LogBaruItem, Warning, TEXT("Server_RequestUseItem Rejected: Invalid Pawn."));
        return;
    }

    BARU_NET_LOG(this, LogBaruItem, Log, TEXT("Server_RequestUseItem Approved for Slot: %d"), SlotIndex);
    // TODO: Item 확인 후 GameplayEffect 적용 및 소모 처리
}

bool ABaruPlayerController::Server_RequestDropItem_Validate(int32 SlotIndex, int32 Count)
{
    return SlotIndex >= 0 && SlotIndex < 100 && Count > 0;
}

void ABaruPlayerController::Server_RequestDropItem_Implementation(int32 SlotIndex, int32 Count)
{
    APawn* ControlledPawn = GetPawn();
    if (!IsValid(ControlledPawn))
    {
        return;
    }

    BARU_NET_LOG(this, LogBaruItem, Log, TEXT("Server_RequestDropItem Approved for Slot: %d, Count: %d"), SlotIndex, Count);
    // TODO: 인벤토리 아이템 제거 및 필드 월드 액터 드롭 스폰 처리
}

bool ABaruPlayerController::Server_SendPing_Validate(FVector PingLocation, EBaruPingType PingType)
{
    return !PingLocation.ContainsNaN();
}

void ABaruPlayerController::Server_SendPing_Implementation(FVector PingLocation, EBaruPingType PingType)
{
    APawn* ControlledPawn = GetPawn();
    if (!IsValid(ControlledPawn))
    {
        return;
    }
    
    const float MaxPingDistSq = FMath::Square(10000.0f);
    if (FVector::DistSquared(ControlledPawn->GetActorLocation(), PingLocation) > MaxPingDistSq)
    {
        BARU_NET_LOG(this, LogBaruNet, Warning, TEXT("Ping Location exceeds maximum allowed range."));
        return;
    }

    BARU_NET_LOG(this, LogBaruNet, Log, TEXT("Relaying Ping: Type %d at %s"), static_cast<int32>(PingType), *PingLocation.ToString());
    
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        if (ABaruPlayerController* TargetPC = Cast<ABaruPlayerController>(Iterator->Get()))
        {
            TargetPC->OnPingReceived.Broadcast(PingLocation, PingType);
        }
    }
}

// Client RPC
void ABaruPlayerController::Client_ShowSettlementUI_Implementation(const FBaruSettlementReport& Report)
{
    BARU_NET_LOG(this, LogBaruUI, Log, TEXT("Client_ShowSettlementUI Received. (Survived: %d, Currency: %d)"), Report.bSurvived, Report.AcquiredCurrency);
    OnSettlementReceived.Broadcast(Report);
}

void ABaruPlayerController::Client_PlayElevatorCinematic_Implementation()
{
    BARU_NET_LOG(this, LogBaruUI, Log, TEXT("Client_PlayElevatorCinematic Received."));
    OnPlayCinematic.Broadcast();
}

