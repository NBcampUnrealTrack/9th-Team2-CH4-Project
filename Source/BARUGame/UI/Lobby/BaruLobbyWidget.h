// BaruLobbyWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/BaruActivatableWidget.h"
#include "Subsystems/BaruSessionSubsystem.h"

#include "BaruLobbyWidget.generated.h"

class UButton;
class UCanvasPanel;
class UListView;
class UTextBlock;
class UWidgetSwitcher;
class UBaruSessionListItemData;
class UBaruLobbyPlayerListItemData;
class ABaruLobbyGameState;
class ABaruPlayerState;

UENUM(BlueprintType)
enum class EBaruLobbyView : uint8
{
	Home,
	ContractSelection,
	SearchLoading,
	SearchResults
};

/**
 * 맵과 계약을 선택하고 팀원을 확인하는
 * 로비 화면의 C++ 기반 클래스
 */
UCLASS(Abstract, Blueprintable)
class BARUGAME_API UBaruLobbyWidget
	: public UBaruActivatableWidget
{
	GENERATED_BODY()
	
public:
	UBaruLobbyWidget(
		const FObjectInitializer& ObjectInitializer);
	
	UFUNCTION(BlueprintCallable, Category = "BARU|UI|Lobby")
	bool ShowLobbyView(EBaruLobbyView NewView);
	
protected:
	virtual void NativeOnInitialized() override;
	
	virtual void NativeOnActivated() override;
	
	virtual void NativeOnDeactivated() override;
	
	UFUNCTION()
	void HandleOpenContractClicked();
	
	UFUNCTION()
	void HandleCloseContractClicked();
	
	UFUNCTION()
	void HandleFindSessionClicked();
	
	UFUNCTION()
	void HandleBackFromSearchResultsClicked();
	
	UFUNCTION()
	void HandleLeaveLobbyClicked();

	UFUNCTION()
	void HandleLobbyActionClicked();

	/**
	 * 방장이 게임 시작을 요청한다.
	 * 실제 서버 이동 연결은 GameMode/PlayerController 담당에서 처리한다.
	 */
	UFUNCTION(
		BlueprintImplementableEvent,
		Category = "BARU|UI|Lobby"
		)
	void BP_OnHostStartGameRequested();
	
	UFUNCTION()
	void HandleFindSessionsComplete(
		const TArray<FBaruSessionSearchResultInfo>& SearchResults,
		bool bWasSuccessful
		);
	
	UFUNCTION()
	void HandleCreateRoomClicked();
	
	UFUNCTION()
	void HandleCreateSessionComplete(
		bool bWasSuccessful
		);
	
	UFUNCTION()
	void HandleTargetMapChanged(
		const FString& NewMapURL
		);
	
	UFUNCTION()
	void HandleAllPlayersReadyChanged(
		bool bAllReady
		);
	
	UFUNCTION()
	void HandleLocalReadyStatusChanged(
		bool bIsReady
		);
	
	void RefreshLobbyActionButton();

	/** 복제된 PlayerArray를 읽어 참가자 ListView를 최신 상태로 맞춘다. */
	void RefreshLobbyPlayerList();
	
	
	void RebuildSessionResultList(
		bool bWasSuccessful
		);
	
	// 화면 패널
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby")
	TObjectPtr<UWidgetSwitcher> Switcher_LobbyView;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby")
	TObjectPtr<UCanvasPanel> Panel_LobbyHome;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby")
	TObjectPtr<UCanvasPanel> Panel_ContractSelection;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby")
	TObjectPtr<UCanvasPanel> Panel_SearchLoading;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby")
	TObjectPtr<UCanvasPanel> Panel_SearchResults;
	
	// BindWidget
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby|Session")
	TObjectPtr<UListView> ListView_SearchResults;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby|Session")
	TObjectPtr<UTextBlock> Text_NoSearchResults;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby|Contract")
	TObjectPtr<UTextBlock> Text_SelectedContractName;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby")
	TObjectPtr<UTextBlock> Text_LobbyAction;

	/** 기존 WBP_Lobby와의 호환을 위해 블루프린트 배치 전까지 선택 바인딩으로 둔다. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "BARU|UI|Lobby|Player")
	TObjectPtr<UListView> ListView_LobbyPlayers;
	
	/**
	 * ListView에 전달한 UObject Item들을 보관한다.
	 * 
	 * 검색을 다시 실행하면 기존 Item을 제거하고
	 * 새로운 검색 결과로 다시 생성한다.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBaruSessionListItemData>>
	SessionListItems;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBaruLobbyPlayerListItemData>>
	LobbyPlayerListItems;
	
	// 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby")
	TObjectPtr<UButton> Button_OpenContract;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby")
	TObjectPtr<UButton> Button_CloseContract;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby")
	TObjectPtr<UButton> Button_FindSession;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby")
	TObjectPtr<UButton> Button_BackFromSearchResults;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby")
	TObjectPtr<UButton> Button_LeaveLobby;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby")
	TObjectPtr<UButton> Button_CreateRoom;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby")
	TObjectPtr<UButton> Button_LobbyAction;


	// 외부 시스템과 검색 데이터
	UPROPERTY(Transient)
	TObjectPtr<UBaruSessionSubsystem> SessionSubsystem;
	
	UPROPERTY(Transient)
	TObjectPtr<ABaruLobbyGameState> LobbyGameState;
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "BARU|UI|Lobby")
	TArray<FBaruSessionSearchResultInfo> SessionSearchResults;
	
	UPROPERTY(Transient)
	TObjectPtr<ABaruPlayerState> LocalPlayerState;
	
	// 방 생성 중복 요청 방지
	UPROPERTY(Transient)
	bool bIsCreatingSession = false;
	
	UPROPERTY(Transient)
	bool bIsLobbyHost = false;

	/** 목록 내용이 실제로 바뀔 때만 ListView를 다시 만들기 위한 비교 값 */
	FString LastLobbyPlayerListSignature;

	FTimerHandle LobbyPlayerListRefreshTimerHandle;
	
	// 현재 화면 상태
	UPROPERTY(Transient, BlueprintReadOnly, Category = "BARU|UI|Lobby")
	EBaruLobbyView CurrentLobbyView = EBaruLobbyView::Home;
};
