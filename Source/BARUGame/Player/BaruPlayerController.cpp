#include "Player/BaruPlayerController.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "Player/BaruPlayerState.h"
#include "AbilitySystem/BaruAbilitySystemComponent.h"   // ★[추가] ProcessAbilityInput 호출용
#include "Subsystems/BaruSaveGameSubsystem.h"
#include "BaruLog.h"
// ★[삭제] #include "EnhancedInputSubsystems.h"
// ★[삭제] #include "Engine/LocalPlayer.h"
//   IMC 등록을 Character 로 일원화했으므로 여기서는 더 이상 필요 없습니다.

ABaruPlayerController::ABaruPlayerController()
{
    PrimaryActorTick.bCanEverTick = true;
    PlayerCameraManagerClass = APlayerCameraManager::StaticClass();
}

void ABaruPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!IsLocalController())   // ★[수정] 중첩 대신 early return
    {
        return;
    }

    FInputModeGameOnly InputModeData;
    InputModeData.SetConsumeCaptureMouseDown(true);
    SetInputMode(InputModeData);
    SetShowMouseCursor(false);
    
    BARU_LOG(LogBaruUI, Log, TEXT("Local PlayerController Initialized: %s"), *GetName());

    // ★[삭제] AddMappingContext 블록 삭제.
    //   ABaruCharacter::SetupPlayerInputComponent 가 동일한 IMC 를 우선순위 0으로 등록하고 있어
    //   이중 등록 상태였습니다. 두 컨텍스트가 나중에 달라지면 같은 키가 두 번 발화합니다.
    //   ※ Character 쪽에는 IMC 미할당 시 에러 로그를 추가해두었습니다.

    // ★[삭제] "TODO : 멀티플레이에서 패킷 전송시 UI 인식이 안되는 것을 예방하는 방지 코드" 주석 삭제
    //   (내용이 없는 미완성 메모라 혼동만 줍니다. 필요하면 이슈로 관리)
}

// ★[추가] GAS 입력 펌프.
//   이 함수는 입력이 존재하는 쪽(소유 클라이언트 / 리슨서버 호스트)에서만 호출되며,
//   TryActivateAbility 가 내부적으로 서버에 활성화 RPC 를 보내므로 이게 정상 GAS 구조입니다.
void ABaruPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
    if (ABaruPlayerState* BaruPS = GetPlayerState<ABaruPlayerState>())
    {
        if (UBaruAbilitySystemComponent* BaruASC = BaruPS->GetBaruAbilitySystemComponent())
        {
            BaruASC->ProcessAbilityInput(DeltaTime, bGamePaused);
        }
    }

    Super::PostProcessInput(DeltaTime, bGamePaused);
}

// Server RPC
bool ABaruPlayerController::Server_RequestEquipItem_Validate(int32 SlotIndex)
{
    return SlotIndex >= 0 && SlotIndex < MaxInventorySlotIndex;   // ★[수정] 매직넘버 100 제거
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
    return SlotIndex >= 0 && SlotIndex < MaxInventorySlotIndex;   // ★[수정]
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
    return SlotIndex >= 0 && SlotIndex < MaxInventorySlotIndex && Count > 0;   // ★[수정]
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
    
    const float MaxPingDistSq = FMath::Square(MaxPingDistance);   // ★[수정] 하드코딩 10000.0f 제거
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
            // ★[수정] 기존: TargetPC->OnPingReceived.Broadcast(PingLocation, PingType);
            //   델리게이트는 복제되지 않아 서버 안에서만 터지고 클라 UI 에는 도달하지 않았습니다.
            TargetPC->Client_ReceivePing(PingLocation, PingType);
        }
    }
}

// Client RPC
void ABaruPlayerController::Client_ShowSettlementUI_Implementation(const FBaruSettlementReport& Report)
{
    BARU_NET_LOG(this, LogBaruUI, Log, TEXT("Client_ShowSettlementUI Received. (Survived: %d, Currency: %d)"), Report.bSurvived, Report.AcquiredCurrency);

    // 로컬 PC의 .sav 에 기록
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UBaruSaveGameSubsystem* SaveSubsystem = GI->GetSubsystem<UBaruSaveGameSubsystem>())
        {
            const FString CurrentPlayerName = PlayerState ? PlayerState->GetPlayerName() : TEXT("Operative");
            SaveSubsystem->RecordRaidResult(CurrentPlayerName, Report.AcquiredCurrency, Report.bSurvived);
        }
    }

    OnSettlementReceived.Broadcast(Report);
}

void ABaruPlayerController::Client_PlayElevatorCinematic_Implementation()
{
    BARU_NET_LOG(this, LogBaruUI, Log, TEXT("Client_PlayElevatorCinematic Received."));
    OnPlayCinematic.Broadcast();
}

// ★[추가] 각 클라이언트 로컬에서 UI 델리게이트를 실제로 터뜨리는 지점
void ABaruPlayerController::Client_ReceivePing_Implementation(FVector_NetQuantize PingLocation, EBaruPingType PingType)
{
    BARU_NET_LOG(this, LogBaruNet, Verbose, TEXT("Client_ReceivePing: Type %d"), static_cast<int32>(PingType));
    OnPingReceived.Broadcast(PingLocation, PingType);
}