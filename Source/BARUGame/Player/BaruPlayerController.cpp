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
#include "UI/Subsystem/BaruUIManagerSubsystem.h"   // [추가] 인벤 UI Push/Pop
#include "UI/BaruUITags.h"                         // [추가] UI_Layer_GameMenu
#include "CommonActivatableWidget.h"               // [추가]
#include "Engine/LocalPlayer.h"                    // [추가] GetSubsystem
#include "BaruLog.h"
#include "UI/ItemUI/BaruItemFocusComponent.h"


ABaruPlayerController::ABaruPlayerController()
{
    PrimaryActorTick.bCanEverTick = true;
    PlayerCameraManagerClass = APlayerCameraManager::StaticClass();
    
    ItemFocusComponent = CreateDefaultSubobject<UBaruItemFocusComponent>(TEXT("ItemFocusComponent"));
    if (ItemFocusComponent)
    {
        ItemFocusComponent->bEnableItemFocusUI = true;
        // Character의 InteractionTraceDistance(기본 300.0f)와 동일하게 일치
        ItemFocusComponent->FocusTraceDistance = 300.0f;
    }
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

//관전 전환 입력 바인딩 사망시 
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
        // [09.13] ESC 바인딩
        if (MenuAction)
        {
            EIC->BindAction(MenuAction, ETriggerEvent::Started, this, &ABaruPlayerController::Input_ToggleGameMenu);
        }
    }
}

// [09.13] 누락되었던 ESC 입력 핸들러 구현부
void ABaruPlayerController::Input_ToggleGameMenu()
{
    ToggleGameMenu();
}

// [09.13] 게임 메뉴 오픈 및 종료 토글 로직
void ABaruPlayerController::ToggleGameMenu()
{
    if (!IsLocalController()) return;

    UBaruUIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UBaruUIManagerSubsystem>(GetLocalPlayer());
    if (!UIManager) return;

    // 1. 이미 메뉴 위젯이 존재하고 활성화되어 있다면 레이어에서 팝(닫기)
    if (IsValid(ActiveGameMenuWidget))
    {
        if (ActiveGameMenuWidget->IsActivated())
        {
            UIManager->PopWidgetFromLayer(BaruUITags::UI_Layer_GameMenu.GetTag());
            ActiveGameMenuWidget = nullptr;
            
            FInputModeGameOnly InputMode;
            InputMode.SetConsumeCaptureMouseDown(true);
            SetInputMode(InputMode);
            SetShowMouseCursor(false);
            return;
        }
        
        // 내부 버튼(계속하기 등)이나 BackHandler로 이미 비활성화된 상태라면 포인터 정리
        ActiveGameMenuWidget = nullptr;
    }

    // 2. 인벤토리가 열려 있는 상태라면 인벤토리만 닫고 즉시 리턴 (메뉴 창 오픈 방지)
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
        ActiveGameMenuWidget->OnDeactivated().AddWeakLambda(this, [this]()
        {
            ActiveGameMenuWidget = nullptr;

            FInputModeGameOnly InputMode;
            InputMode.SetConsumeCaptureMouseDown(true);
            SetInputMode(InputMode);
            SetShowMouseCursor(false);

            BARU_LOG(LogBaruUI, Log, TEXT("GameMenu Deactivated -> Restored InputModeGameOnly."));
        });
    }
}

// [09.13] IsActivated() 분기로 대체되었으므로 비워두거나 안전용으로 유지
void ABaruPlayerController::HandleGameMenuDeactivated()
{
    ActiveGameMenuWidget = nullptr;
}

// [추가] 입력 핸들러. 클라에서 실행되며 서버에 요청만 보냄
void ABaruPlayerController::Input_SpectateNext()
{
    Server_CycleSpectatorTarget(true);
}

void ABaruPlayerController::Input_SpectatePrev()
{
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

// ★[추가 09.10] 인벤토리 토글 입력.
void ABaruPlayerController::Input_ToggleInventory()
{
    ToggleInventory();
}

// ★[추가 09.10] 인벤토리 UI 열기/닫기.
void ABaruPlayerController::ToggleInventory()
{
    if (!IsLocalController())
    {
        return;
    }

    UBaruUIManagerSubsystem* UIManager =
        ULocalPlayer::GetSubsystem<UBaruUIManagerSubsystem>(GetLocalPlayer());

    if (!UIManager)
    {
        BARU_LOG(LogBaruUI, Warning, TEXT("ToggleInventory: UIManagerSubsystem 을 찾지 못했습니다."));
        return;
    }

    // ── 이미 열려 있으면 닫기 ────────────────────────────────
    if (IsValid(ActiveInventoryWidget))
    {
        UIManager->PopWidgetFromLayer(BaruUITags::UI_Layer_GameMenu.GetTag());
        ActiveInventoryWidget = nullptr;

        // 게임 입력으로 복귀
        FInputModeGameOnly InputMode;
        InputMode.SetConsumeCaptureMouseDown(true);
        SetInputMode(InputMode);
        SetShowMouseCursor(false);

        BARU_LOG(LogBaruUI, Log, TEXT("Inventory closed."));
        return;
    }

    // ── 닫혀 있으면 열기 ────────────────────────────────────
    if (!InventoryWidgetClass)
    {
        BARU_LOG(LogBaruUI, Warning,
            TEXT("ToggleInventory: BP_BaruPlayerController 에 Inventory Widget Class 가 비어 있습니다."));
        return;
    }

    ActiveInventoryWidget =
        UIManager->PushWidgetToLayer(BaruUITags::UI_Layer_GameMenu.GetTag(), InventoryWidgetClass);

    if (!ActiveInventoryWidget)
    {
        BARU_LOG(LogBaruUI, Warning, TEXT("ToggleInventory: 위젯 생성에 실패했습니다."));
        return;
    }

    // 마우스로 아이템을 옮겨야 하므로 커서를 켜고 UI 입력을 허용합니다.
    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(ActiveInventoryWidget->TakeWidget());
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockOnCapture);
    SetInputMode(InputMode);
    SetShowMouseCursor(true);

    BARU_LOG(LogBaruUI, Log, TEXT("Inventory opened."));
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

bool ABaruPlayerController::Server_CycleSpectatorTarget_Validate(bool bNext)
{
    // _Validate 에서 false 를 반환하면 해당 클라이언트의 연결이 끊깁니다.
    // 파라미터가 bool 하나뿐이라 조작 가능한 값이 없으므로 항상 통과시키고,
    // "관전 자격이 있는가" 같은 게임 규칙은 아래 _Implementation 에서 거릅니다.
    return true;
}

void ABaruPlayerController::Server_CycleSpectatorTarget_Implementation(bool bNext)
{
    // [1] 자격 검사 — 살아있는 플레이어가 팀원 시점을 훔쳐보는 것을 차단합니다.
    //     추출 호러에서 벽 너머 몬스터 위치를 아는 건 치명적인 이득이라 반드시 막아야 합니다.
    const ABaruPlayerState* MyPS = GetPlayerState<ABaruPlayerState>();
    if (!MyPS || MyPS->IsAlive())
    {
        BARU_NET_LOG(this, LogBaruSession, Warning, TEXT("CycleSpectatorTarget rejected: requester is still alive."));
        return;
    }

    // [2] 관전 가능한 대상 수집
    TArray<APawn*> Candidates;
    GatherSpectatablePawns(Candidates);

    if (Candidates.Num() == 0)
    {
        BARU_NET_LOG(this, LogBaruSession, Log, TEXT("CycleSpectatorTarget: no living teammates to spectate."));
        return;
    }

    // [3] 현재 대상의 위치를 찾고 다음/이전으로 이동 (순환)
    int32 CurrentIndex = Candidates.IndexOfByKey(CurrentSpectatingPawn.Get());
    if (CurrentIndex == INDEX_NONE)
    {
        // 보고 있던 대상이 죽었거나 나갔으면 첫 번째부터 시작
        CurrentIndex = 0;
    }
    else
    {
        const int32 Delta = bNext ? 1 : -1;
        CurrentIndex = (CurrentIndex + Delta + Candidates.Num()) % Candidates.Num();
        //                             Num() 을 더하는 이유: bNext=false 일 때 음수 인덱스 방지
    }

    APawn* NewTarget = Candidates[CurrentIndex];
    CurrentSpectatingPawn = NewTarget;

    
    SetViewTargetWithBlend(NewTarget, 0.5f);

    if (!IsLocalController())
    {
        ClientSetViewTarget(NewTarget, FViewTargetTransitionParams());
    }

    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Spectating Target Changed to: %s"), *GetNameSafe(NewTarget));
}

// [추가] 살아있는 팀원의 폰만 모읍니다. 서버에서만 호출됩니다.
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
            continue;   // 자기 자신은 제외
        }

        if (!BaruPS->IsAlive())
        {
            continue;   // 죽은 팀원은 관전 대상이 아님
        }

        if (APawn* SpectatablePawn = BaruPS->GetPawn())
        {
            OutPawns.Add(SpectatablePawn);
        }
    }
}

// [추가 09.04]
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

    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("StartRaid approved. Target: %s"),
        *LobbyGS->GetSelectedTargetMapURL());
    
    LobbyGM->StartGameRaid();
}

//  탐사 목표 맵 선택
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
    // [1] 방장 확인
    if (!IsLocalController())
    {
        BARU_NET_LOG(this, LogBaruSession, Warning, TEXT("SetTargetRaidMap rejected: requester is not the host."));
        return;
    }

    // [2] 지금이 로비인지
    ABaruLobbyGameMode* LobbyGM = Cast<ABaruLobbyGameMode>(World->GetAuthGameMode());
    if (!LobbyGM)
    {
        BARU_NET_LOG(this, LogBaruSession, Warning, TEXT("SetTargetRaidMap rejected: current GameMode is not a lobby."));
        return;
    }

    // [3] 허용된 맵인지 검사
    //   클라이언트가 보낸 문자열을 그대로 믿으면 임의의 레벨로 팀 전체를 끌고 갈 수 있습니다.
    //   목록이 비어 있으면 아직 설정 전이므로, 조용히 통과시키지 않고 거부합니다.
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

    // [4] 확정. GameState 복제와 UI 갱신은 GameMode 가 처리합니다.
    LobbyGM->SetTargetRaidMap(TargetMapURL);

    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Target raid map set to: %s"), *TargetMapURL);
}