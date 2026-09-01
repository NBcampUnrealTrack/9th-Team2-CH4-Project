// BaurLobbyWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/BaruActivatableWidget.h"
#include "Subsystems/BaruSessionSubsystem.h"

#include "BaruLobbyWidget.generated.h"

class UButton;
class UCanvasPanel;
class UWidgetSwitcher;

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
	void HandleFindSessionsComplete(
		const TArray<FBaruSessionSearchResultInfo>& SearchResults,
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
	
	// 버튼
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby")
	TObjectPtr<UButton> Button_OpenContract;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby")
	TObjectPtr<UButton> Button_CloseContract;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby")
	TObjectPtr<UButton> Button_FindSession;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby")
	TObjectPtr<UButton> Button_BackFromSearchResults;
	
	// 외부 시스템과 검색 데이터
	UPROPERTY(Transient)
	TObjectPtr<UBaruSessionSubsystem> SessionSubsystem;
	
	UPROPERTY(Transient, BlueprintReadOnly, Category = "BARU|UI|Lobby")
	TArray<FBaruSessionSearchResultInfo> SessionSearchResults;
	
	// 현재 화면 상태
	UPROPERTY(Transient, BlueprintReadOnly, Category = "BARU|UI|Lobby")
	EBaruLobbyView CurrentLobbyView = EBaruLobbyView::Home;
};
