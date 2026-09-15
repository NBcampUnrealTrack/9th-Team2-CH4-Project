#include "Player/BaruPlayerController.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "EnhancedInputComponent.h"       
#include "GameFramework/GameStateBase.h" 
#include "Player/BaruPlayerState.h"
#include "Core/BaruLobbyGameMode.h"    
#include "Core/BaruLobbyGameState.h"   
#include "AbilitySystem/BaruAbilitySystemComponent.h"   
#include "Subsystems/BaruSaveGameSubsystem.h"
#include "UI/Subsystem/BaruUIManagerSubsystem.h"
#include "UI/BaruUITags.h"
#include "CommonActivatableWidget.h"
#include "Engine/LocalPlayer.h"
#include "BaruLog.h"
#include "UI/ItemUI/BaruItemFocusComponent.h"
#include "Framework/Application/SlateApplication.h"

ABaruPlayerController::ABaruPlayerController()
{
    PrimaryActorTick.bCanEverTick = true;
    PlayerCameraManagerClass = APlayerCameraManager::StaticClass();
    
    ItemFocusComponent = CreateDefaultSubobject<UBaruItemFocusComponent>(TEXT("ItemFocusComponent"));
    if (ItemFocusComponent)
    {
        ItemFocusComponent->bEnableItemFocusUI = true;
        ItemFocusComponent->FocusTraceDistance = 300.0f;
    }
}

void ABaruPlayerController::BeginPlay()
{
    Super::BeginPlay();

    if (!IsLocalController())
    {
        return;
    }

    FInputModeGameOnly InputModeData;
    InputModeData.SetConsumeCaptureMouseDown(false);
    SetInputMode(InputModeData);
    SetShowMouseCursor(false);
    
    // [09.13] 세이브 파일에서 확정된 닉네임을 PlayerState에 전달
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UBaruSaveGameSubsystem* SaveSubsystem = GI->GetSubsystem<UBaruSaveGameSubsystem>())
        {
            const FString SavedName = SaveSubsystem->GetActiveProfilePlayerName();
            if (!SavedName.IsEmpty())
            {
                if (ABaruPlayerState* PS = GetPlayerState<ABaruPlayerState>())
                {
                    PS->Server_UpdateNickname(SavedName);
                }
            }
        }
    }
    
    BARU_LOG(LogBaruUI, Log, TEXT("Local PlayerController Initialized: %s"), *GetName());
}

void ABaruPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
    {
        if (SpectateNextAction)
        {
            EIC->BindAction(SpectateNextAction, ETriggerEvent::Started, this, &ABaruPlayerController::Input_SpectateNext);
        }
        if (SpectatePrevAction)
        {
            EIC->BindAction(SpectatePrevAction, ETriggerEvent::Started, this, &ABaruPlayerController::Input_SpectatePrev);
        }
        if (ToggleInventoryAction)
        {
            EIC->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &ABaruPlayerController::Input_ToggleInventory);
        }
        if (MenuAction)
        {
            EIC->BindAction(MenuAction, ETriggerEvent::Started, this, &ABaruPlayerController::Input_ToggleGameMenu);
        }
    }
}

void ABaruPlayerController::Input_ToggleGameMenu()
{
    ToggleGameMenu();
}

void ABaruPlayerController::ToggleGameMenu()
{
    if (!IsLocalController()) return;
    
    // [09.14] UI 키 연타 방지하여 안전성 보장
    const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    if (CurrentTime - LastGameMenuToggleTime < 0.2f) return;
    LastGameMenuToggleTime = CurrentTime;

    UBaruUIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UBaruUIManagerSubsystem>(GetLocalPlayer());
    if (!UIManager) return;

    // 1. 이미 메뉴 위젯이 존재하고 활성화되어 있다면 레이어에서 팝(닫기)
    if (IsValid(ActiveGameMenuWidget))
    {
        if (ActiveGameMenuWidget->IsActivated())
        {
            UIManager->PopWidgetFromLayer(BaruUITags::UI_Layer_GameMenu.GetTag());
            ActiveGameMenuWidget = nullptr;
            
            // [수정 09.14] ESC 메뉴 닫힐 때 FPS 1인칭 게임 입력 복구
            FInputModeGameOnly InputMode;
            InputMode.SetConsumeCaptureMouseDown(false);
            SetInputMode(InputMode);
            SetShowMouseCursor(false);
            
            // [09.14] 닫힐 때 뷰포트로 키보드 포커스 강제 회수
            if (FSlateApplication::IsInitialized())
            {
                FSlateApplication::Get().SetAllUserFocusToGameViewport();
            }
            return;
        }
        
        ActiveGameMenuWidget = nullptr;
    }

    // 2. 인벤토리가 열려 있는 상태라면 인벤토리만 닫고 즉시 리턴
    if (IsValid(ActiveInventoryWidget))
    {
        ToggleInventory();
        return;
    }

    // 3. 게임 메뉴 위젯 클래스 검증
    if (!GameMenuWidgetClass)
    {
        BARU_LOG(LogBaruUI, Warning, TEXT("ToggleGameMenu: BP_BaruPlayerController에 GameMenuWidgetClass가 비어 있습니다."));
        return;
    }

    // 4. UI Manager를 통해 GameMenu 레이어에 푸시
    ActiveGameMenuWidget = UIManager->PushWidgetToLayer(BaruUITags::UI_Layer_GameMenu.GetTag(), GameMenuWidgetClass);
    
    if (IsValid(ActiveGameMenuWidget))
    {
        // [수정 09.14] 내부 버튼(계속하기 등)으로 닫힐 때도 1인칭 FPS 조작으로 복귀
        ActiveGameMenuWidget->OnDeactivated().AddWeakLambda(this, [this]()
        {
            ActiveGameMenuWidget = nullptr;

            FInputModeGameOnly InputMode;
            InputMode.SetConsumeCaptureMouseDown(false);
            SetInputMode(InputMode);
            SetShowMouseCursor(false);

            // [09.14] 버튼 클릭 후 슬레이트 포커스가 공중에 뜨는 현상 방지
            if (FSlateApplication::IsInitialized())
            {
                FSlateApplication::Get().SetAllUserFocusToGameViewport();
            }
            
            BARU_LOG(LogBaruUI, Log, TEXT("GameMenu Deactivated -> Restored InputModeGameOnly."));
        });
    }
}

void ABaruPlayerController::HandleGameMenuDeactivated()
{
    ActiveGameMenuWidget = nullptr;
}

void ABaruPlayerController::Input_SpectateNext()
{
    const ABaruPlayerState* MyPS = GetPlayerState<ABaruPlayerState>();
    if (!MyPS || !MyPS->IsDead())
    {
        return;
    }
    Server_CycleSpectatorTarget(true);
}

void ABaruPlayerController::Input_SpectatePrev()
{
    const ABaruPlayerState* MyPS = GetPlayerState<ABaruPlayerState>();
    if (!MyPS || !MyPS->IsDead())
    {
        return;
    }
    Server_CycleSpectatorTarget(false);
}

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

void ABaruPlayerController::Input_ToggleInventory()
{
    ToggleInventory();
}

// [수정 09.14] 인벤토리 UI 열기/닫기 및 1인칭 FPS 포커스 복원 로직
void ABaruPlayerController::ToggleInventory()
{
    if (!IsLocalController())
    {
        return;
    }
    
    // [09.14] 더블 트리거(0.1초 만에 닫히고 다시 열리는 레이스 컨디션) 방지 쿨타임 (0.25초)
    const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    if (CurrentTime - LastInventoryToggleTime < 0.25f)
    {
        return;
    }
    LastInventoryToggleTime = CurrentTime;

    UBaruUIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UBaruUIManagerSubsystem>(GetLocalPlayer());
    if (!UIManager)
    {
        BARU_LOG(LogBaruUI, Warning, TEXT("ToggleInventory: UIManagerSubsystem 을 찾지 못했습니다."));
        return;
    }

    // 1. 이미 열려 있으면 닫기 (I 키 토글 닫기)
    if (IsValid(ActiveInventoryWidget))
    {
        if (ActiveInventoryWidget->IsActivated())
        {
            UIManager->PopWidgetFromLayer(BaruUITags::UI_Layer_GameMenu.GetTag());
            ActiveInventoryWidget = nullptr;

            // [수정 09.14] 닫히는 즉시 완벽한 1인칭 FPS 조작 모드로 복귀 (마우스 커서 숨김)
            FInputModeGameOnly InputMode;
            InputMode.SetConsumeCaptureMouseDown(false);
            SetInputMode(InputMode);
            SetShowMouseCursor(false);

            // [09.14] 1인칭 FPS 뷰포트로 키보드 포커스 즉시 강제 회수
            if (FSlateApplication::IsInitialized())
            {
                FSlateApplication::Get().SetAllUserFocusToGameViewport();
            }
            
            BARU_LOG(LogBaruUI, Log, TEXT("Inventory closed -> Restored FPS InputMode."));
            return;
        }

        ActiveInventoryWidget = nullptr;
    }

    // 2. 닫혀 있으면 열기
    if (!InventoryWidgetClass)
    {
        BARU_LOG(LogBaruUI, Warning, TEXT("ToggleInventory: BP_BaruPlayerController 에 Inventory Widget Class 가 비어 있습니다."));
        return;
    }

    ActiveInventoryWidget = UIManager->PushWidgetToLayer(BaruUITags::UI_Layer_GameMenu.GetTag(), InventoryWidgetClass);
    if (!ActiveInventoryWidget)
    {
        BARU_LOG(LogBaruUI, Warning, TEXT("ToggleInventory: 위젯 생성에 실패했습니다."));
        return;
    }

    // [수정 09.14] 위젯 내부(NativeOnKeyDown 등)에서 비활성화될 때도 FPS 조작으로 복귀 보장
    ActiveInventoryWidget->OnDeactivated().AddWeakLambda(this, [this]()
    {
        ActiveInventoryWidget = nullptr;

        FInputModeGameOnly InputMode;
        InputMode.SetConsumeCaptureMouseDown(false);
        SetInputMode(InputMode);
        SetShowMouseCursor(false);

        if (FSlateApplication::IsInitialized())
        {
            FSlateApplication::Get().SetAllUserFocusToGameViewport();
        }

        BARU_LOG(LogBaruUI, Log, TEXT("Inventory Deactivated -> Restored FPS Viewport Focus."));
    });

    // [수정 09.14] SetWidgetToFocus를 절대 호출하지 않고, 키 입력을 뷰포트에 남겨 'I' 키를 다시 인식하도록 설정
    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(ActiveInventoryWidget->TakeWidget());
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);
    SetShowMouseCursor(true);

    BARU_LOG(LogBaruUI, Log, TEXT("Inventory opened."));
}

// =============================================================================
// Server RPC 구현부
// [수정 09.14] 헤더의 UFUNCTION(Server, Reliable, WithValidation) 선언과 완벽히 일치하도록 매칭
// =============================================================================

bool ABaruPlayerController::Server_RequestEquipItem_Validate(int32 SlotIndex)
{
    return SlotIndex >= 0 && SlotIndex < MaxInventorySlotIndex;
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
}

bool ABaruPlayerController::Server_RequestUseItem_Validate(int32 SlotIndex)
{
    return SlotIndex >= 0 && SlotIndex < MaxInventorySlotIndex;
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
}

bool ABaruPlayerController::Server_RequestDropItem_Validate(int32 SlotIndex, int32 Count)
{
    return SlotIndex >= 0 && SlotIndex < MaxInventorySlotIndex && Count > 0;
}

void ABaruPlayerController::Server_RequestDropItem_Implementation(int32 SlotIndex, int32 Count)
{
    APawn* ControlledPawn = GetPawn();
    if (!IsValid(ControlledPawn))
    {
        return;
    }

    BARU_NET_LOG(this, LogBaruItem, Log, TEXT("Server_RequestDropItem Approved for Slot: %d, Count: %d"), SlotIndex, Count);
}

// Client RPC
void ABaruPlayerController::Client_ShowSettlementUI_Implementation(const FBaruSettlementReport& Report)
{
    BARU_NET_LOG(this, LogBaruUI, Log, TEXT("Client_ShowSettlementUI Received. (Survived: %d, Currency: %d)"), Report.bSurvived, Report.AcquiredCurrency);

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

void ABaruPlayerController::Client_InteractionHoldStarted_Implementation(AActor* OtherActor, float Duration, bool bIsHolder)
{
    BARU_NET_LOG(this, LogBaruUI, Log, TEXT("Interaction hold started. Other=%s Duration=%.2f Holder=%d"),
        *GetNameSafe(OtherActor), Duration, bIsHolder);
    OnInteractionHoldStarted.Broadcast(OtherActor, Duration, bIsHolder);
}

void ABaruPlayerController::Client_InteractionHoldEnded_Implementation(AActor* OtherActor, EBaruInteractionHoldEndReason Reason, bool bIsHolder)
{
    BARU_NET_LOG(this, LogBaruUI, Log, TEXT("Interaction hold ended. Other=%s Reason=%s Holder=%d"),
        *GetNameSafe(OtherActor), *UEnum::GetValueAsString(Reason), bIsHolder);
    OnInteractionHoldEnded.Broadcast(OtherActor, Reason, bIsHolder);
}

bool ABaruPlayerController::Server_CycleSpectatorTarget_Validate(bool bNext)
{
    return true;
}

void ABaruPlayerController::Server_CycleSpectatorTarget_Implementation(bool bNext)
{
    const ABaruPlayerState* MyPS = GetPlayerState<ABaruPlayerState>();
    if (!MyPS || !MyPS->IsDead())
    {
        BARU_NET_LOG(this, LogBaruSession, Warning, TEXT("CycleSpectatorTarget rejected: requester is not dead."));
        return;
    }

    TArray<APawn*> Candidates;
    GatherSpectatablePawns(Candidates);

    if (Candidates.Num() == 0)
    {
        BARU_NET_LOG(this, LogBaruSession, Log, TEXT("CycleSpectatorTarget: no living teammates to spectate."));
        // 생존 팀원이 없으면 빈 문자열 통지 (UI 숨김)
        Client_NotifySpectatingTargetChanged(TEXT(""));
        return;
    }

    int32 CurrentIndex = Candidates.IndexOfByKey(CurrentSpectatingPawn.Get());
    if (CurrentIndex == INDEX_NONE)
    {
        CurrentIndex = 0;
    }
    else
    {
        const int32 Delta = bNext ? 1 : -1;
        CurrentIndex = (CurrentIndex + Delta + Candidates.Num()) % Candidates.Num();
    }

    APawn* NewTarget = Candidates[CurrentIndex];
    CurrentSpectatingPawn = NewTarget;

    SetViewTargetWithBlend(NewTarget, 0.5f);

    if (!IsLocalController())
    {
        ClientSetViewTarget(NewTarget, FViewTargetTransitionParams());
    }
    
    // 관전 대상의 PlayerState에서 확정된 닉네임 추출 후 클라이언트에 통지
    FString TargetName = TEXT("작전 대원");
    if (IsValid(NewTarget))
    {
        if (const ABaruPlayerState* TargetPS = NewTarget->GetPlayerState<ABaruPlayerState>())
        {
            const FString BaseName = TargetPS->GetPlayerName().IsEmpty() 
                ? FString::Printf(TEXT("대원 %d"), TargetPS->GetPlayerId()) 
                : TargetPS->GetPlayerName();

            TargetName = TargetPS->IsDBNO() 
                ? FString::Printf(TEXT("%s (구조 대기)"), *BaseName) 
                : BaseName;
        }
    }
    Client_NotifySpectatingTargetChanged(TargetName);

    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Spectating Target Changed to: %s"), *GetNameSafe(NewTarget));
}

void ABaruPlayerController::GatherSpectatablePawns(TArray<APawn*>& OutPawns) const
{
    OutPawns.Reset();

    AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
    if (!GS)
    {
        return;
    }

    for (APlayerState* PS : GS->PlayerArray)
    {
        const ABaruPlayerState* BaruPS = Cast<ABaruPlayerState>(PS);
        if (!BaruPS || BaruPS == GetPlayerState<ABaruPlayerState>())
        {
            continue;
        }

        if (BaruPS->IsDead())
        {
            continue;
        }

        APawn* SpectatablePawn = BaruPS->GetPawn();
        if (IsValid(SpectatablePawn) && !SpectatablePawn->IsPendingKillPending())
        {
            OutPawns.Add(SpectatablePawn);
        }
    }
}

bool ABaruPlayerController::Server_RequestStartRaid_Validate()
{
    return true;
}

void ABaruPlayerController::Server_RequestStartRaid_Implementation()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    
    if (!IsLocalController())
    {
        BARU_NET_LOG(this, LogBaruSession, Warning, TEXT("StartRaid rejected: requester is not the host."));
        return;
    }

    ABaruLobbyGameMode* LobbyGM = Cast<ABaruLobbyGameMode>(World->GetAuthGameMode());
    if (!LobbyGM)
    {
        BARU_NET_LOG(this, LogBaruSession, Warning, TEXT("StartRaid rejected: current GameMode is not a lobby."));
        return;
    }

    const ABaruLobbyGameState* LobbyGS = World->GetGameState<ABaruLobbyGameState>();
    if (!LobbyGS)
    {
        BARU_NET_LOG(this, LogBaruSession, Warning, TEXT("StartRaid rejected: LobbyGameState not found."));
        return;
    }

    if (!LobbyGS->IsAllPlayersReady())
    {
        BARU_NET_LOG(this, LogBaruSession, Log, TEXT("StartRaid rejected: not all players are ready."));
        return;
    }

    if (LobbyGS->GetSelectedTargetMapURL().IsEmpty())
    {
        BARU_NET_LOG(this, LogBaruSession, Warning, TEXT("StartRaid rejected: no target map selected."));
        return;
    }

    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("StartRaid approved. Target: %s"), *LobbyGS->GetSelectedTargetMapURL());
    LobbyGM->StartGameRaid();
}

bool ABaruPlayerController::Server_RequestSetTargetRaidMap_Validate(const FString& TargetMapURL)
{
    return !TargetMapURL.IsEmpty() && TargetMapURL.Len() <= 260;
}

void ABaruPlayerController::Server_RequestSetTargetRaidMap_Implementation(const FString& TargetMapURL)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (!IsLocalController())
    {
        BARU_NET_LOG(this, LogBaruSession, Warning, TEXT("SetTargetRaidMap rejected: requester is not the host."));
        return;
    }

    ABaruLobbyGameMode* LobbyGM = Cast<ABaruLobbyGameMode>(World->GetAuthGameMode());
    if (!LobbyGM)
    {
        BARU_NET_LOG(this, LogBaruSession, Warning, TEXT("SetTargetRaidMap rejected: current GameMode is not a lobby."));
        return;
    }

    if (AllowedRaidMapURLs.Num() == 0)
    {
        BARU_NET_LOG(this, LogBaruSession, Warning,
            TEXT("SetTargetRaidMap rejected: AllowedRaidMapURLs is empty. BP_BaruPlayerController 에 맵 목록을 설정하세요."));
        return;
    }

    if (!AllowedRaidMapURLs.Contains(TargetMapURL))
    {
        BARU_NET_LOG(this, LogBaruSession, Warning,
            TEXT("SetTargetRaidMap rejected: '%s' is not in the allowed list."), *TargetMapURL);
        return;
    }

    LobbyGM->SetTargetRaidMap(TargetMapURL);
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Target raid map set to: %s"), *TargetMapURL);
}

void ABaruPlayerController::PostSeamlessTravel()
{
    Super::PostSeamlessTravel();

    // 로컬 컨트롤러(호스트 및 클라이언트 각자의 머신)에서 즉시 입력 상태 복원
    if (IsLocalController())
    {
        Client_ResetLobbyInputAndState_Implementation();
    }
}

void ABaruPlayerController::Client_ResetLobbyInputAndState_Implementation()
{
    // 1. 관전 및 입력 대기(Waiting) 플래그 완전 해제 (키 입력 드랍 방지)
    bPlayerIsWaiting = false;
    ChangeState(NAME_Playing);

    if (PlayerState)
    {
        PlayerState->SetIsOnlyASpectator(false);
    }

    ResetIgnoreMoveInput();
    ResetIgnoreLookInput();
    CurrentSpectatingPawn.Reset();
    
    // 로비 복귀 시 켜져 있던 관전 UI 닫기
    OnSpectatorTargetChanged.Broadcast(TEXT(""));

    // 2. 이전 맵에서 남은 UI 포인터 및 디바운스 타이머 초기화
    ActiveGameMenuWidget = nullptr;
    ActiveInventoryWidget = nullptr;
    LastGameMenuToggleTime = 0.0f;
    LastInventoryToggleTime = 0.0f;

    // 3. 1인칭 FPS 조작 모드로 복귀 및 마우스 커서 숨김
    FInputModeGameOnly InputMode;
    InputMode.SetConsumeCaptureMouseDown(false);
    SetInputMode(InputMode);
    SetShowMouseCursor(false);

    // 4. 슬레이트 포커스를 게임 뷰포트로 강제 회수
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().SetAllUserFocusToGameViewport();
    }

    BARU_LOG(LogBaruUI, Log, TEXT("Client_ResetLobbyInputAndState: Spectator state cleared and Game Viewport focus restored."));
}

// 관전 대상 닉네임 수신 Client RPC 구현
void ABaruPlayerController::Client_NotifySpectatingTargetChanged_Implementation(const FString& SpectatedPlayerName)
{
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Spectating Target Name Received: %s"), *SpectatedPlayerName);
    OnSpectatorTargetChanged.Broadcast(SpectatedPlayerName);
}