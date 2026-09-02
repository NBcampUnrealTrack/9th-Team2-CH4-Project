// BaruSessionEntryWidget.cpp

#include "UI/Lobby/BaruSessionEntryWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"

#include "BaruLog.h"
#include "Subsystems/BaruSessionSubsystem.h"
#include "UI/Lobby/BaruSessionListItemData.h"

void UBaruSessionEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	
	if (IsValid(Button_JoinSession))
	{
		Button_JoinSession->OnClicked.AddDynamic(
			this,
			&UBaruSessionEntryWidget::HandleJoinSessionClicked
			);
	}
}

void UBaruSessionEntryWidget::NativeOnListItemObjectSet(
	UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(
		ListItemObject
		);
	
	ItemData =
		Cast<UBaruSessionListItemData>(ListItemObject);
	
	if (!IsValid(ItemData))
	{
		BARU_LOG(
			LogBaruUI,
			Error,
			TEXT("세션 ListView Item Data 형식이 올바르지 않습니다.")
			);
		
		return;
	}
	
	const FBaruSessionSearchResultInfo& SessionInfo =
		ItemData->SessionInfo;
	
	Text_ServerName->SetText(
		FText::FromString(SessionInfo.ServerName));
	
	Text_HostPlayerName->SetText(
		FText::FromString(SessionInfo.HostPlayerName));
	
	Text_SelectedMapName->SetText(
		FText::FromString(SessionInfo.SelectedMapName));
	
	Text_PlayerCount->SetText(
		FText::FromString(FString::Printf(TEXT("%d / %d"),
			SessionInfo.CurrentPlayers,
			SessionInfo.MaxPlayers)));
	
	Text_Ping->SetText(
		FText::FromString(
			FString::Printf(
				TEXT("%d ms"),
				SessionInfo.PingInMs)));
	
}

void UBaruSessionEntryWidget::HandleJoinSessionClicked()
{
	if (!IsValid(ItemData))
	{
		return;
	}
	
	UGameInstance* GameInstance = GetGameInstance();
	if (!IsValid(GameInstance))
	{
		return;
	}
	
	UBaruSessionSubsystem* SessionSubsystem =
		GameInstance->GetSubsystem<UBaruSessionSubsystem>();
	
	if (!IsValid(SessionSubsystem))
	{
		BARU_LOG(
			LogBaruUI,
			Error,
			TEXT("방 참가 요청에 필요한 SessionSubsystem이 없습니다.")
			);
		
		return;
	}
	
	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("세션 참가를 요청합니다. Index=%d, ServerName=%s"),
		ItemData->SessionInfo.SessionIndex,
		*ItemData->SessionInfo.ServerName
		);
	
	SessionSubsystem->JoinSessionByIndex(
		ItemData->SessionInfo.SessionIndex
		);
}
