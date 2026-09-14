// BaruLobbyWidget.cpp

#include "UI/Lobby/BaruLobbyWidget.h"

#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/WidgetSwitcher.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "TimerManager.h"

#include "BaruLog.h"
#include "Core/BaruLobbyGameState.h"
#include "Player/BaruPlayerState.h"
#include "UI/Lobby/BaruSessionListItemData.h"
#include "UI/Lobby/BaruLobbyPlayerListItemData.h"
#include "UI/Lobby/BaruContractListItemData.h"

UBaruLobbyWidget::UBaruLobbyWidget(
    const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    /**
     * 로비 화면도 캐릭터 조작 없이 
     * 마우스와 UI만 사용한다.
     */
    InputConfig = EBaruWidgetInputMode::Menu;
    
    GameMouseCaptureMode = EMouseCaptureMode::NoCapture;
}

void UBaruLobbyWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    
    if (IsValid(Button_OpenContract))
    {
       Button_OpenContract->OnClicked.AddDynamic(
          this,
          &UBaruLobbyWidget::HandleOpenContractClicked
          );
    }
    
    if (IsValid(Button_CloseContract))
    {
       Button_CloseContract->OnClicked.AddDynamic(
          this,
          &UBaruLobbyWidget::HandleCloseContractClicked
          );
    }
    
    if (IsValid(Button_ConfirmContract))
    {
       Button_ConfirmContract->OnClicked.AddDynamic(
          this,
          &ThisClass::HandleConfirmContractClicked
          );
       
       Button_ConfirmContract->SetIsEnabled(false);
    }
    
    if (IsValid(Button_FindSession))
    {
       Button_FindSession->OnClicked.AddDynamic(
          this,
          &UBaruLobbyWidget::HandleFindSessionClicked
          );
    }
    
    if (IsValid(Button_BackFromSearchResults))
    {
       Button_BackFromSearchResults->OnClicked.AddDynamic(
          this,
          &UBaruLobbyWidget::HandleBackFromSearchResultsClicked
          );
    }
    
    if (IsValid(Button_LeaveLobby))
    {
       Button_LeaveLobby->OnClicked.AddDynamic(
          this,
          &UBaruLobbyWidget::HandleLeaveLobbyClicked
          );
    }

    if (IsValid(Button_CreateRoom))
    {
       Button_CreateRoom->OnClicked.AddDynamic(
          this,
          &ThisClass::HandleCreateRoomClicked);
    }
    
    if (IsValid(Button_LobbyAction))
    {
       Button_LobbyAction->OnClicked.AddDynamic(
          this,
          &ThisClass::HandleLobbyActionClicked);
    }
}

// 화면 전환 함수
bool UBaruLobbyWidget::ShowLobbyView(EBaruLobbyView NewView)
{
    if (!IsValid(Switcher_LobbyView))
    {
       BARU_LOG(
          LogBaruUI,
          Error,
          TEXT("Lobby WidgetSwitcher가 유효하지 않습니다.")
          );
       
       return false;
    }
    
    UCanvasPanel* TargetPanel;
    
    switch (NewView)
    {
    case EBaruLobbyView::Home:
       TargetPanel = Panel_LobbyHome;
       break;
       
    case EBaruLobbyView::ContractSelection:
       TargetPanel = Panel_ContractSelection;
       break;
       
    case EBaruLobbyView::SearchLoading:
       TargetPanel = Panel_SearchLoading;
       break;
       
    case EBaruLobbyView::SearchResults:
       TargetPanel = Panel_SearchResults;
       break;
       
    default:
       return false;
    }
    
    if (!IsValid(TargetPanel))
    {
       BARU_LOG(
          LogBaruUI,
          Error,
          TEXT("전환할 Lobby Panel이 유효하지 않습니다.")
          );
       
       return false;
    }
    
    Switcher_LobbyView->SetActiveWidget(TargetPanel);
    CurrentLobbyView = NewView;

    // [수정] 검색 관련 화면을 벗어나면 대기/검색 중이던 타이머를 즉시 정리하여 불필요한 백그라운드 호출 차단
    if (NewView != EBaruLobbyView::SearchResults && NewView != EBaruLobbyView::SearchLoading)
    {
       if (UWorld* World = GetWorld())
       {
          World->GetTimerManager().ClearTimer(SearchTimeoutTimerHandle);
          World->GetTimerManager().ClearTimer(AutoRefreshSessionsTimerHandle);
       }
    }
    
    // [수정] StaticEnum 컴파일러 삭제 에러(= delete C2280)를 원천 차단하는 안전한 문자열 변환 적용
    FString ViewName;
    switch (NewView)
    {
    case EBaruLobbyView::Home: ViewName = TEXT("Home"); break;
    case EBaruLobbyView::ContractSelection: ViewName = TEXT("ContractSelection"); break;
    case EBaruLobbyView::SearchLoading: ViewName = TEXT("SearchLoading"); break;
    case EBaruLobbyView::SearchResults: ViewName = TEXT("SearchResults"); break;
    default: ViewName = TEXT("Unknown"); break;
    }
    
    BARU_LOG(
       LogBaruUI,
       Log,
       TEXT("Lobby 화면을 전환했습니다. View=%s"),
       *ViewName
       );
    
    return true;
}

void UBaruLobbyWidget::HandleOpenContractClicked()
{
    ShowLobbyView(EBaruLobbyView::ContractSelection);
}

void UBaruLobbyWidget::HandleCloseContractClicked()
{
    ShowLobbyView(EBaruLobbyView::Home);
}

void UBaruLobbyWidget::RebuildContractList()
{
    if (!IsValid(ListView_Contracts))
    {
       BARU_LOG(
          LogBaruUI,
          Error,
          TEXT("계약 목록 ListView가 유효하지 않습니다.")
          );
       
       return;
    }
    
    ListView_Contracts->ClearListItems();
    ContractListItems.Reset();
    SelectedContractItem = nullptr;
    
    if (IsValid(Button_ConfirmContract))
    {
       Button_ConfirmContract->SetIsEnabled(false);
    }
    
    for (const FBaruContractDefinition& Definition
       : ContractDefinitions)
    {
       if (Definition.ContractName.IsEmpty()
          || Definition.TargetMapURL.IsEmpty())
       {
          BARU_LOG(
             LogBaruUI,
             Warning,
             TEXT("이름 또는 맵 URL이 비어 있는 계약을 건너뜁니다.")
             );
          
          continue;
       }
       
       UBaruContractListItemData* NewItem =
          NewObject<UBaruContractListItemData>(this);
       
       if (!IsValid(NewItem))
       {
          continue;
       }
       
       NewItem->Initialize(
          Definition.ContractName,
          Definition.MapName,
          Definition.Difficulty,
          Definition.RewardText,
          Definition.TargetMapURL,
          Definition.Thumbnail.Get()
          );
       
       NewItem->OnSelected.AddDynamic(
          this,
          &ThisClass::HandleContractSelected
          );
       
       ContractListItems.Add(NewItem);
       ListView_Contracts->AddItem(NewItem);
    }
    
    BARU_LOG(
       LogBaruUI,
       Log,
       TEXT("계약 목록을 구성했습니다. Count=%d"),
       ContractListItems.Num()
       );
}

void UBaruLobbyWidget::HandleContractSelected(
    UBaruContractListItemData* SelectedContract)
{
    if (!IsValid(SelectedContract))
    {
       return;
    }
    
    SelectedContractItem = SelectedContract;
    
    if (IsValid(ListView_Contracts))
    {
       ListView_Contracts->SetSelectedItem(
          SelectedContract
          );
    }
    
    if (IsValid(Text_ContractDetailName))
    {
       Text_ContractDetailName->SetText(
          SelectedContract->ContractName
          );
    }
    
    if (IsValid(Text_ContractDetailMapName))
    {
       Text_ContractDetailMapName->SetText(
          SelectedContract->MapName
          );
    }
    
    if (IsValid(Text_ContractDetailDifficulty))
    {
       Text_ContractDetailDifficulty->SetText(
          SelectedContract->Difficulty
          );
    }
    
    if (IsValid(Text_ContractDetailReward))
    {
       Text_ContractDetailReward->SetText(
          SelectedContract->RewardText
          );
    }
    
    if (IsValid(Image_ContractDetailThumbnail))
    {
       if (IsValid(SelectedContract->Thumbnail.Get()))
       {
          Image_ContractDetailThumbnail->SetBrushFromTexture(
             SelectedContract->Thumbnail.Get()
             );
          
          Image_ContractDetailThumbnail->SetVisibility(
             ESlateVisibility::Visible
             );
       }
       else
       {
          Image_ContractDetailThumbnail->SetVisibility(
             ESlateVisibility::Collapsed
             );
       }
    }
    
    if (IsValid(Button_ConfirmContract))
    {
       Button_ConfirmContract->SetIsEnabled(
          bIsLobbyHost
          );
    }
    
    BARU_LOG(
       LogBaruUI,
       Log,
       TEXT("계약 상세 정보를 갱신했습니다. Contract=%s, map=%s"),
       *SelectedContract->ContractName.ToString(),
       *SelectedContract->TargetMapURL
       );
}

void UBaruLobbyWidget::HandleConfirmContractClicked()
{
    if (!bIsLobbyHost)
    {
       BARU_LOG(
          LogBaruUI,
          Warning,
          TEXT("방장만 계약을 확정할 수 있습니다.")
          );
       
       return;
    }
    
    if (!IsValid(SelectedContractItem))
    {
       BARU_LOG(
          LogBaruUI,
          Warning,
          TEXT("확정할 계약이 선택되지 않았습니다.")
          );
       
       return;
    }
    
    BP_OnContractConfirmedRequested(
       SelectedContractItem->TargetMapURL
       );
    
    if (IsValid(Text_SelectedContractName))
    {
       Text_SelectedContractName->SetText(
          SelectedContractItem->ContractName
          );
    }
    
    if (IsValid(Text_SelectedMapName))
    {
       Text_SelectedMapName->SetText(
          SelectedContractItem->MapName
          );
    }
    
    if (IsValid(Text_SelectedDifficulty))
    {
       Text_SelectedDifficulty->SetText(
          SelectedContractItem->Difficulty
          );
    }

   UpdateSelectedMapPreview(
      SelectedContractItem->Thumbnail.Get()
      );
    
    BARU_LOG(
       LogBaruUI,
       Log,
       TEXT("계약 선택을 확정했습니다. Contract=%s, Map=%s"),
       *SelectedContractItem->ContractName.ToString(),
       *SelectedContractItem->TargetMapURL
       );
    
    ShowLobbyView(EBaruLobbyView::Home);
}

void UBaruLobbyWidget::HandleFindSessionClicked()
{
    BARU_LOG(
       LogBaruUI,
       Log,
       TEXT("매칭 버튼이 클릭되어 세션 검색을 요청합니다."));

    if (!IsValid(SessionSubsystem))
    {
       BARU_LOG(
          LogBaruUI,
          Error,
          TEXT("SessionSubsystem을 찾지 못했습니다.")
          );

       return;
    }
    
    ShowLobbyView(EBaruLobbyView::SearchLoading);
    
    // [수정] 스팀 검색 무응답/지연 대비 8초 타임아웃 타이머 가동
    if (UWorld* World = GetWorld())
    {
       World->GetTimerManager().ClearTimer(SearchTimeoutTimerHandle);
       World->GetTimerManager().SetTimer(
          SearchTimeoutTimerHandle,
          this,
          &ThisClass::OnSearchTimeout,
          8.0f,
          false);
    }

    SessionSubsystem->FindSessions(
       50,
       false
       );
}

void UBaruLobbyWidget::HandleBackFromSearchResultsClicked()
{
    // [수정] 세션 결과 화면 나갈 때 자동 갱신 및 타임아웃 타이머 취소
    if (UWorld* World = GetWorld())
    {
       World->GetTimerManager().ClearTimer(SearchTimeoutTimerHandle);
       World->GetTimerManager().ClearTimer(AutoRefreshSessionsTimerHandle);
    }

    ShowLobbyView(EBaruLobbyView::Home);
}

void UBaruLobbyWidget::HandleLeaveLobbyClicked()
{
    if (!IsValid(SessionSubsystem))
    {
       BARU_LOG(
          LogBaruUI,
          Error,
          TEXT("로비를 나갈 SessionSubsystem을 찾지 못했습니다.")
          );
       return;
    }

    // [수정] 로비 퇴장 시 동작 중인 모든 백그라운드 폴링/타이머 즉시 정리
    if (UWorld* World = GetWorld())
    {
       World->GetTimerManager().ClearTimer(LobbyPlayerListRefreshTimerHandle);
       World->GetTimerManager().ClearTimer(StateBindRetryTimerHandle);
       World->GetTimerManager().ClearTimer(SearchTimeoutTimerHandle);
       World->GetTimerManager().ClearTimer(AutoRefreshSessionsTimerHandle);
    }

    if (IsValid(Button_LeaveLobby))
    {
       Button_LeaveLobby->SetIsEnabled(false);
    }

    BARU_LOG(
       LogBaruUI,
       Log,
       TEXT("로비 나가기를 요청했습니다.")
       );

    SessionSubsystem->DestroySession(true);
}

void UBaruLobbyWidget::HandleFindSessionsComplete(
    const TArray<FBaruSessionSearchResultInfo>& SearchResults,
    bool bWasSuccessful)
{
    // [수정] 세션 결과가 응답되었으므로 타임아웃 타이머 해제
    if (UWorld* World = GetWorld())
    {
       World->GetTimerManager().ClearTimer(SearchTimeoutTimerHandle);
    }

    SessionSearchResults = SearchResults;
    
    RebuildSessionResultList(
       bWasSuccessful
       );
    
    BARU_LOG(
       LogBaruUI,
       Log,
       TEXT("세션 검색 결과를 수신했습니다. Success=%d, Count=%d"),
       bWasSuccessful,
       SessionSearchResults.Num()
       );
    
    ShowLobbyView(EBaruLobbyView::SearchResults);

    // [수정] 검색 결과 목록 화면에 체류 중일 때 5초 주기로 백그라운드 자동 새로고침 가동
    if (UWorld* World = GetWorld())
    {
       World->GetTimerManager().ClearTimer(AutoRefreshSessionsTimerHandle);
       World->GetTimerManager().SetTimer(
          AutoRefreshSessionsTimerHandle,
          this,
          &ThisClass::RequestSilentSessionRefresh,
          5.0f,
          true);
    }
}

void UBaruLobbyWidget::HandleCreateRoomClicked()
{
    if (bIsCreatingSession)
    {
       return;
    }
    
    if (!IsValid(SessionSubsystem))
    {
       BARU_LOG(
          LogBaruUI,
          Error,
          TEXT("방을 생성할 SessionSubsystem을 찾지 못했습니다.")
          );
       return;
    }
    
    bIsCreatingSession = true;
    
    if (IsValid(Button_CreateRoom))
    {
       Button_CreateRoom->SetIsEnabled(false);
    }
    
    BARU_LOG(
       LogBaruUI,
       Log,
       TEXT("방 생성을 요청했습니다.")
       );
    
    SessionSubsystem->CreateSession(
       5,
       false,
       TEXT("BARU Room")
       );
}

void UBaruLobbyWidget::HandleCreateSessionComplete(
    bool bWasSuccessful)
{
    bIsCreatingSession = false;
    
    if (IsValid(Button_CreateRoom))
    {
       Button_CreateRoom->SetIsEnabled(true);
    }
    
    if (bWasSuccessful)
    {
       BARU_LOG(
          LogBaruUI,
          Log,
          TEXT("방 생성에 성공했습니다.")
          );
    }
    else
    {
       BARU_LOG(
          LogBaruUI,
          Error,
          TEXT("방 생성에 실패했습니다.")
          );
    }
}

void UBaruLobbyWidget::HandleTargetMapChanged(
    const FString& NewMapURL)
{
    // 같은 맵을 서로 다른 URL 형식으로 받아도 비교할 수 있게 정리한다.
    const auto NormalizeMapURL =
       [](FString MapURL)
       {
          int32 OptionIndex = INDEX_NONE;
          
          if (MapURL.FindChar(
             TEXT('?'),
             OptionIndex))
          {
             MapURL.LeftInline(OptionIndex);
          }
          
          int32 DotIndex = INDEX_NONE;
          
          if (MapURL.FindChar(
             TEXT('.'),
             DotIndex))
          {
             MapURL.LeftInline(DotIndex);
          }
          
          return MapURL;
       };
    
    const FString NormalizedMapURL =
       NormalizeMapURL(NewMapURL);
    
    // 아직 선택된 맵이 없는 상태
    if (NormalizedMapURL.IsEmpty())
    {
       if (IsValid(Text_SelectedContractName))
       {
          Text_SelectedContractName->SetText(
             FText::FromString(
                TEXT("선택된 계약 없음"))
                );
       }
       
       if (IsValid(Text_SelectedMapName))
       {
          Text_SelectedMapName->SetText(
             FText::GetEmpty()
             );
       }
       
       if (IsValid(Text_SelectedDifficulty))
       {
          Text_SelectedDifficulty->SetText(
             FText::GetEmpty()
             );
       }
       
       UpdateSelectedMapPreview(nullptr);

       return;
    }
    
    // 수신한 맵 URL과 일치하는 계약 데이터를 찾는다.
    const FBaruContractDefinition* MatchedDefinition =
       ContractDefinitions.FindByPredicate(
          [&NormalizeMapURL, &NormalizedMapURL](
             const FBaruContractDefinition& Definition)
          {
             return NormalizeMapURL(
                Definition.TargetMapURL
                ).Equals(
                   NormalizedMapURL,
                   ESearchCase::IgnoreCase
                   );
          });
    
    if (MatchedDefinition != nullptr)
    {
       if (IsValid(Text_SelectedContractName))
       {
          Text_SelectedContractName->SetText(
             MatchedDefinition->ContractName
             );
       }
       
       if (IsValid(Text_SelectedMapName))
       {
          Text_SelectedMapName->SetText(
             MatchedDefinition->MapName
             );
       }
       
       if (IsValid(Text_SelectedDifficulty))
       {
          Text_SelectedDifficulty->SetText(
             MatchedDefinition->Difficulty
             );
       }
       
       UpdateSelectedMapPreview(
          MatchedDefinition->Thumbnail.Get()
          );

       BARU_LOG(
          LogBaruUI,
          Log,
          TEXT("서버의 맵 선택을 계약 UI에 반영했습니다. Contract=%s, Map=%s"),
          *MatchedDefinition->ContractName.ToString(),
          *NormalizedMapURL
          );
       
       return;
    }
    
    // 계약 목록에 없는 맵이 전달된 경우 URL 마지막 이름을 임시 표시한다.
    FString FallbackName = NormalizedMapURL;
    int32 LastSlashIndex = INDEX_NONE;
    
    if (FallbackName.FindLastChar(
       TEXT('/'),
       LastSlashIndex))
    {
       FallbackName.RightChopInline(
          LastSlashIndex + 1
          );
    }
    
    if (IsValid(Text_SelectedContractName))
    {
       Text_SelectedContractName->SetText(
          FText::FromString(FallbackName)
          );
    }
    
    if (IsValid(Text_SelectedMapName))
    {
       Text_SelectedMapName->SetText(
          FText::GetEmpty()
          );
    }
    
    if (IsValid(Text_SelectedDifficulty))
    {
       Text_SelectedDifficulty->SetText(
          FText::GetEmpty()
          );
    }
    
    UpdateSelectedMapPreview(nullptr);

    BARU_LOG(
       LogBaruUI,
       Warning,
       TEXT("맵 URL과 일치하는 계약 데이터를 찾지 못했습니다. Map=%s"),
       *NormalizedMapURL
       );
}

void UBaruLobbyWidget::HandleAllPlayersReadyChanged(
    bool bAllReady)
{
    RefreshLobbyActionButton();
    
    BARU_LOG(
       LogBaruUI,
       Log,
       TEXT("전체 준비 상태를 UI에 반영했습니다. AllReady=%d"),
       bAllReady);
}

void UBaruLobbyWidget::HandleLobbyActionClicked()
{
    if (bIsLobbyHost)
    {
       if (!IsValid(LobbyGameState) ||
          !LobbyGameState->IsAllPlayersReady())
       {
          BARU_LOG(
             LogBaruUI,
             Warning,
             TEXT("아직 모든 참가자가 준비되지 않아 게임을 시작할 수 없습니다."));
          return;
       }
       
       BARU_LOG(
          LogBaruUI,
          Log,
          TEXT("방장이 게임 시작을 요청했습니다."));
       
       BP_OnHostStartGameRequested();
       return;
    }
    
    if (!IsValid(LocalPlayerState))
    {
       BARU_LOG(
          LogBaruUI,
          Error,
          TEXT("준비 상태를 변경할 LocalPlayerState가 없습니다."));
       return;
    }
    
    const bool bNewReady =
       !LocalPlayerState->IsReady();
    
    BARU_LOG(
       LogBaruUI,
       Log,
       TEXT("플레이어 준비 상태 변경을 요청했습니다. NewReady=%d"),
       bNewReady);
    
    LocalPlayerState->Server_SetReadyStatus(
       bNewReady);
}

void UBaruLobbyWidget::HandleLocalReadyStatusChanged(
    bool bIsReady)
{
    RefreshLobbyActionButton();
    
    BARU_LOG(
       LogBaruUI,
       Log,
       TEXT("로컬 플레이어 준비 상태 UI를 갱신했습니다. Ready=%d"),
       bIsReady);
}

void UBaruLobbyWidget::RefreshLobbyActionButton()
{
    if (!IsValid(Button_LobbyAction) ||
       !IsValid(Text_LobbyAction))
    {
       return;
    }
    
    if (bIsLobbyHost)
    {
       Text_LobbyAction->SetText(
          FText::FromString(TEXT("게임 시작")));
       
       const bool bCanStartGame =
          IsValid(LobbyGameState) &&
             LobbyGameState->IsAllPlayersReady();
       
       Button_LobbyAction->SetIsEnabled(
          bCanStartGame);
       
       return;
    }
    
    const bool bIsReady =
       IsValid(LocalPlayerState) &&
          LocalPlayerState->IsReady();
    
    Text_LobbyAction->SetText(
       FText::FromString(
          bIsReady
          ? TEXT("준비 취소")
          : TEXT("준비")));
    
    // 참가자는 준비/준비 취소를 위해 항상 클릭 가능
    Button_LobbyAction->SetIsEnabled(
       IsValid(LocalPlayerState));
}

void UBaruLobbyWidget::RefreshLobbyPlayerList()
{
    if (!IsValid(ListView_LobbyPlayers) || !IsValid(LobbyGameState))
    {
       return;
    }
    
    // [수정] 0.5초 주기마다 액션 버튼의 활성/준비 상태도 함께 검증하여 이벤트 누락 방어
    RefreshLobbyActionButton();

    TArray<ABaruPlayerState*> LobbyPlayers;
    for (APlayerState* PlayerState : LobbyGameState->PlayerArray)
    {
       if (ABaruPlayerState* BaruPlayerState =
          Cast<ABaruPlayerState>(PlayerState))
       {
          LobbyPlayers.Add(BaruPlayerState);
       }
    }

    LobbyPlayers.Sort([](
       const ABaruPlayerState& Left,
       const ABaruPlayerState& Right)
    {
       return Left.GetPlayerId() < Right.GetPlayerId();
    });

    FString NewSignature;
    for (const ABaruPlayerState* PlayerState : LobbyPlayers)
    {
       NewSignature += FString::Printf(
          TEXT("%d|%s|%d;"),
          PlayerState->GetPlayerId(),
          *PlayerState->GetPlayerName(),
          PlayerState->IsReady());
    }

    if (NewSignature == LastLobbyPlayerListSignature)
    {
       return;
    }

    LastLobbyPlayerListSignature = MoveTemp(NewSignature);
    ListView_LobbyPlayers->ClearListItems();
    LobbyPlayerListItems.Reset();

    for (int32 PlayerIndex = 0;
       PlayerIndex < LobbyPlayers.Num();
       ++PlayerIndex)
    {
       ABaruPlayerState* PlayerState = LobbyPlayers[PlayerIndex];
       FString PlayerName = PlayerState->GetPlayerName();
       if (PlayerName.IsEmpty())
       {
          PlayerName = FString::Printf(
             TEXT("Player %d"),
             PlayerIndex + 1);
       }

       UBaruLobbyPlayerListItemData* NewItem =
          NewObject<UBaruLobbyPlayerListItemData>(this);
       if (!IsValid(NewItem))
       {
          continue;
       }

       // 현재 공용 코드에는 방장 ID가 없으므로 가장 먼저 접속한 PlayerId를 방장으로 표시한다.
       NewItem->Initialize(
          PlayerName,
          PlayerIndex == 0,
          PlayerState->IsReady(),
          PlayerState == LocalPlayerState);

       LobbyPlayerListItems.Add(NewItem);
       ListView_LobbyPlayers->AddItem(NewItem);
    }

    BARU_LOG(
       LogBaruUI,
       Log,
       TEXT("로비 참가자 목록을 갱신했습니다. Count=%d"),
       LobbyPlayerListItems.Num());
}

void UBaruLobbyWidget::RebuildSessionResultList(
    bool bWasSuccessful)
{
    if (!IsValid(ListView_SearchResults) ||
       !IsValid(Text_NoSearchResults))
    {
       BARU_LOG(
          LogBaruUI,
          Error,
          TEXT("세션 검색 결과를 표시할 ListView 또는 Text가 없습니다.")
          );
       return;
    }
    
    // 이전 검색 결과 제거
    ListView_SearchResults->ClearListItems();
    SessionListItems.Reset();
    
    if (!bWasSuccessful)
    {
       Text_NoSearchResults->SetText(
          FText::FromString(
             TEXT("방 검색에 실패했습니다.")
             )
             );
       ListView_SearchResults->SetVisibility(
          ESlateVisibility::Collapsed);
       
       Text_NoSearchResults->SetVisibility(
          ESlateVisibility::Visible);
       return;
    }
    
    for (const FBaruSessionSearchResultInfo& SearchResult : SessionSearchResults)
    {
       UBaruSessionListItemData* NewItem =
          NewObject<UBaruSessionListItemData>(this);
       
       if (!IsValid(NewItem))
       {
          continue;
       }
       
       NewItem->Initialize(
          SearchResult
          );
       
       SessionListItems.Add(
          NewItem
          );
       
       ListView_SearchResults->AddItem(
          NewItem
          );
    }
    
    const bool bHasSearchResults =
       SessionListItems.Num() > 0;
    
    ListView_SearchResults->SetVisibility(
       bHasSearchResults
       ? ESlateVisibility::Visible
       : ESlateVisibility::Collapsed
       );
    Text_NoSearchResults->SetVisibility(
       bHasSearchResults
       ? ESlateVisibility::Collapsed
       : ESlateVisibility::Visible
       );
    
    if (!bHasSearchResults)
    {
       Text_NoSearchResults->SetText(
          FText::FromString(
             TEXT("검색된 방이 없습니다.")
             )
             );
    }
}

void UBaruLobbyWidget::NativeOnActivated()
{
    Super::NativeOnActivated();
    
    if (UGameInstance* GameInstance = GetGameInstance())
    {
       SessionSubsystem =
          GameInstance->GetSubsystem<UBaruSessionSubsystem>();
    }
    
    if (IsValid(SessionSubsystem))
    {
       SessionSubsystem->OnFindSessionsCompleteEvent.RemoveDynamic(
          this,
          &UBaruLobbyWidget::HandleFindSessionsComplete
          );
       
       SessionSubsystem->OnFindSessionsCompleteEvent.AddDynamic(
          this,
          &UBaruLobbyWidget::HandleFindSessionsComplete
          );
       
       SessionSubsystem->OnCreateSessionCompleteEvent.RemoveDynamic(
          this,
          &ThisClass::HandleCreateSessionComplete
          );
       
       SessionSubsystem->OnCreateSessionCompleteEvent.AddDynamic(
          this,
          &ThisClass::HandleCreateSessionComplete
          );
    }
    
    // 복제 지연에 완벽 대응하는 지속 바인딩 안전장치 호출
    TryBindLobbyStates();
    
    RefreshLobbyActionButton();
    RefreshLobbyPlayerList();
    RebuildContractList();

    // [수정] 0.5초 주기 타이머로 플레이어 목록 및 상태 강제 동기화 (Watchdog 역할)
    if (UWorld* World = GetWorld())
    {
       World->GetTimerManager().SetTimer(
          LobbyPlayerListRefreshTimerHandle,
          this,
          &ThisClass::RefreshLobbyPlayerList,
          0.5f,
          true);
    }
    
    ShowLobbyView(EBaruLobbyView::Home);
    
    BARU_LOG(
       LogBaruUI,
       Log,
       TEXT("로비 UI가 활성화되었습니다. Widget=%s"),
       *GetName()
       );
}

void UBaruLobbyWidget::NativeOnDeactivated()
{
    // 비활성화 시 모든 안전장치 타이머 일괄 정리
    if (UWorld* World = GetWorld())
    {
       World->GetTimerManager().ClearTimer(LobbyPlayerListRefreshTimerHandle);
       World->GetTimerManager().ClearTimer(StateBindRetryTimerHandle);
       World->GetTimerManager().ClearTimer(SearchTimeoutTimerHandle);
       World->GetTimerManager().ClearTimer(AutoRefreshSessionsTimerHandle);
    }

    LastLobbyPlayerListSignature.Reset();
    LobbyPlayerListItems.Reset();
    
    if (IsValid(ListView_Contracts))
    {
       ListView_Contracts->ClearListItems();
    }
    
    ContractListItems.Reset();
    SelectedContractItem = nullptr;

    if (IsValid(SessionSubsystem))
    {
       SessionSubsystem->OnFindSessionsCompleteEvent.RemoveDynamic(
          this,
          &UBaruLobbyWidget::HandleFindSessionsComplete
          );
       
       SessionSubsystem->OnCreateSessionCompleteEvent.RemoveDynamic(
          this,
          &ThisClass::HandleCreateSessionComplete
          );
    }
    
    SessionSubsystem = nullptr;
    
    if (IsValid(LobbyGameState))
    {
       LobbyGameState->OnTargetMapChanged.RemoveDynamic(
          this,
          &ThisClass::HandleTargetMapChanged);
       
       LobbyGameState->OnAllPlayersReadyChanged.RemoveDynamic(
          this,
          &ThisClass::HandleAllPlayersReadyChanged);
    }
    
    if (IsValid(LocalPlayerState))
    {
       LocalPlayerState->OnReadyStatusChanged.RemoveDynamic(
          this,
          &ThisClass::HandleLocalReadyStatusChanged);
    }
    
    LocalPlayerState = nullptr;
    bIsLobbyHost = false;
    
    LobbyGameState = nullptr;
    
    BARU_LOG(
       LogBaruUI,
       Log,
       TEXT("로비 UI가 비활성화 되었습니다. Widget=%s"),
       *GetName()
       );
    
    Super::NativeOnDeactivated();
}

// 복제 지연을 극복하기 위한 안전장치 폴링 바인딩 구현부
void UBaruLobbyWidget::TryBindLobbyStates()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // 1. GameState 바인딩 시도
    if (!IsValid(LobbyGameState))
    {
       LobbyGameState = World->GetGameState<ABaruLobbyGameState>();
       if (IsValid(LobbyGameState))
       {
          LobbyGameState->OnTargetMapChanged.AddUniqueDynamic(this, &ThisClass::HandleTargetMapChanged);
          LobbyGameState->OnAllPlayersReadyChanged.AddUniqueDynamic(this, &ThisClass::HandleAllPlayersReadyChanged);
            
          HandleTargetMapChanged(LobbyGameState->GetSelectedTargetMapURL());
          HandleAllPlayersReadyChanged(LobbyGameState->IsAllPlayersReady());
       }
    }

    // 2. LocalPlayerState 바인딩 시도
    if (!IsValid(LocalPlayerState))
    {
       if (APlayerController* OwningPlayer = GetOwningPlayer())
       {
          bIsLobbyHost = OwningPlayer->HasAuthority();
          LocalPlayerState = OwningPlayer->GetPlayerState<ABaruPlayerState>();
            
          if (IsValid(LocalPlayerState))
          {
             LocalPlayerState->OnReadyStatusChanged.AddUniqueDynamic(this, &ThisClass::HandleLocalReadyStatusChanged);
             HandleLocalReadyStatusChanged(LocalPlayerState->IsReady());
          }
       }
    }

    // 두 객체가 모두 준비되었을 때만 재시도 타이머를 끄고 UI 최종 동기화
    if (IsValid(LobbyGameState) && IsValid(LocalPlayerState))
    {
       World->GetTimerManager().ClearTimer(StateBindRetryTimerHandle);
       RefreshLobbyActionButton();
       RefreshLobbyPlayerList();
    }
    else
    {
       // 아직 복제 전이라면 0.2초 간격으로 폴링 재시도
       World->GetTimerManager().SetTimer(
          StateBindRetryTimerHandle,
          this,
          &ThisClass::TryBindLobbyStates,
          0.2f,
          false);
    }
}

// 방 검색 무응답 시 자동 갱신을 끄고 로비 홈으로 복귀하는 타임아웃 구현
void UBaruLobbyWidget::OnSearchTimeout()
{
    if (UWorld* World = GetWorld())
    {
       World->GetTimerManager().ClearTimer(AutoRefreshSessionsTimerHandle);
    }

    BARU_LOG(LogBaruUI, Warning, TEXT("방 검색 타임아웃 발생. 로비 홈으로 복귀합니다."));
    ShowLobbyView(EBaruLobbyView::Home);
}

// 검색 결과 화면을 보고 있을 때 5초 주기로 방 목록을 재검색하는 백그라운드 갱신 구현
void UBaruLobbyWidget::RequestSilentSessionRefresh()
{
    if (CurrentLobbyView == EBaruLobbyView::SearchResults && IsValid(SessionSubsystem))
    {
       SessionSubsystem->FindSessions(50, false);
    }
}

void UBaruLobbyWidget::UpdateSelectedMapPreview(
   UTexture2D* Thumbnail)
{
   if (!IsValid(Image_SelectedMapPreview))
   {
      return;
   }

   if (IsValid(Thumbnail))
   {
      Image_SelectedMapPreview->SetBrushFromTexture(
         Thumbnail);

      Image_SelectedMapPreview->SetVisibility(
         ESlateVisibility::HitTestInvisible);

      return;
   }

   Image_SelectedMapPreview->SetBrushFromTexture(
      nullptr);

   Image_SelectedMapPreview->SetVisibility(
      ESlateVisibility::Collapsed);
}
