// BaruLobbyWidget.cpp

#include "UI/Lobby/BaruLobbyWidget.h"

#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/WidgetSwitcher.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"

#include "GameFramework/PlayerController.h"

#include "BaruLog.h"
#include "Core/BaruLobbyGameState.h"
#include "Player/BaruPlayerState.h"
#include "UI/Lobby/BaruSessionListItemData.h"

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
	
	UCanvasPanel* TargetPanel = nullptr;
	
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
	
	const FString ViewName =
		StaticEnum<EBaruLobbyView>()->GetNameStringByValue(static_cast<int64>(NewView));
	
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

void UBaruLobbyWidget::HandleFindSessionClicked()
{
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
	
	SessionSubsystem->FindSessions(
		50,
		false
		);
}

void UBaruLobbyWidget::HandleBackFromSearchResultsClicked()
{
	ShowLobbyView(EBaruLobbyView::Home);
}

void UBaruLobbyWidget::HandleFindSessionsComplete(
	const TArray<FBaruSessionSearchResultInfo>& SearchResults,
	bool bWasSuccessful)
{
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
	if (!IsValid(Text_SelectedContractName))
	{
		return;
	}
	
	FString ContractName = NewMapURL;
	
	/**
	 * "/Game/BaruGame/Maps.Targym"에서
	 * 마지막 부분인 "TextGym"만 꺼낸다
	 */
	int32 LastSlashIndex = INDEX_NONE;
	
	if (ContractName.FindLastChar(
		TEXT('/'),
		LastSlashIndex))
	{
		ContractName.RightChopInline(
			LastSlashIndex + 1);
	}
	
	// 오브젝트 경로에 ".TestGym" 같은 부분이 붙어 있다면 제거한다.
	int32 DotIndex = INDEX_NONE;
	
	if (ContractName.FindChar(
		TEXT('.'),
		DotIndex))
	{
		ContractName.LeftInline(DotIndex);
	}
	
	if (ContractName.IsEmpty())
	{
		ContractName = TEXT("선택된 계약 없음");
	}
	
	Text_SelectedContractName->SetText(
		FText::FromString(ContractName));
	
	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("선택된 계약 UI를 갱신했습니다. Map=%s"),
		*NewMapURL);
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
	
	if (UWorld* World = GetWorld())
	{
		LobbyGameState =
			World->GetGameState<ABaruLobbyGameState>();
	}
	
	if (IsValid(LobbyGameState))
	{
		// 재활성화될 때 중복 등록되는 것을 방지
		LobbyGameState->OnTargetMapChanged.RemoveDynamic(
			this,
			&ThisClass::HandleTargetMapChanged);
		
		LobbyGameState->OnTargetMapChanged.AddDynamic(
			this,
			&ThisClass::HandleTargetMapChanged);
		
		LobbyGameState->OnAllPlayersReadyChanged.RemoveDynamic(
			this,
			&ThisClass::HandleAllPlayersReadyChanged);
		
		LobbyGameState->OnAllPlayersReadyChanged.AddDynamic(
			this,
			&ThisClass::HandleAllPlayersReadyChanged);
		
		// UI가 만들어지기 전에 값이 이미 설정됐을 수 있으므로
		// 현재 상태를 즉시 한 번 반영한다.
		HandleTargetMapChanged(
			LobbyGameState->GetSelectedTargetMapURL());
		
		HandleAllPlayersReadyChanged(
			LobbyGameState->IsAllPlayersReady());
	}
	else
	{
		BARU_LOG(
			LogBaruUI,
			Warning,
			TEXT("BaruLobbyGameState를 찾지 못했습니다."));
	}
	
	if (APlayerController* OwningPlayer =
		GetOwningPlayer())
	{
		bIsLobbyHost =
			OwningPlayer->HasAuthority();
		
		LocalPlayerState =
			OwningPlayer->GetPlayerState<ABaruPlayerState>();
	}
	
	if (IsValid(LocalPlayerState))
	{
		LocalPlayerState->OnReadyStatusChanged.RemoveDynamic(
			this,
			&ThisClass::HandleLocalReadyStatusChanged);
		
		LocalPlayerState->OnReadyStatusChanged.AddDynamic(
			this,
			&ThisClass::HandleLocalReadyStatusChanged);
		
		HandleLocalReadyStatusChanged(
			LocalPlayerState->IsReady());
	}
	else
	{
		BARU_LOG(
			LogBaruUI,
			Warning,
			TEXT("로컬 BaruPlayerState를 찾지 못했습니다."));
	}
	
	RefreshLobbyActionButton();
	
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
