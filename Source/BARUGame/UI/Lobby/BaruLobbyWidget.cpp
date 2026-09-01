// BaruLobbyWidget.cpp

#include "UI/Lobby/BaruLobbyWidget.h"

#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/WidgetSwitcher.h"

#include "Engine/GameInstance.h"

#include "BaruLog.h"

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
	
	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("세션 검색 결과를 수신했습니다. Success=%d, Count=%d"),
		bWasSuccessful,
		SessionSearchResults.Num()
		);
	
	ShowLobbyView(EBaruLobbyView::SearchResults);
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
	if (IsValid(SessionSubsystem))
	{
		SessionSubsystem->OnFindSessionsCompleteEvent.RemoveDynamic(
			this,
			&UBaruLobbyWidget::HandleFindSessionsComplete
			);
	}
	
	SessionSubsystem = nullptr;
	
	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("로비 UI가 비활성화 되었습니다. Widget=%s"),
		*GetName()
		);
	
	Super::NativeOnDeactivated();
}
