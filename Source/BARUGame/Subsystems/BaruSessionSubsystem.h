#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "UObject/SoftObjectPtr.h"
#include "Engine/EngineBaseTypes.h"
#include "BaruSessionSubsystem.generated.h"

class UWorld;
class UNetDriver;

namespace BaruMatchmakingConstants
{
    const FName SETTING_MATCH_KEY = FName(TEXT("BARU_MATCH_KEY"));
    const FString BARU_MATCH_KEY_VALUE = TEXT("BARU_EXTRACTION_HORROR_V1");
    const FName SETTING_SERVER_NAME = FName(TEXT("SERVER_NAME"));
    const FName SETTING_MAP_NAME = FName(TEXT("MAP_NAME"));
    const FName SETTING_HOST_NAME = FName(TEXT("HOST_NAME"));
}

USTRUCT(BlueprintType)
struct FBaruSessionSearchResultInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Session")
    int32 SessionIndex = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Session")
    FString ServerName = TEXT("Unknown Room");

    UPROPERTY(BlueprintReadOnly, Category = "Session")
    FString HostPlayerName = TEXT("Unknown Host");

    UPROPERTY(BlueprintReadOnly, Category = "Session")
    FString SelectedMapName = TEXT("MainLobbyLevel");

    UPROPERTY(BlueprintReadOnly, Category = "Session")
    int32 CurrentPlayers = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Session")
    int32 MaxPlayers = 5;

    UPROPERTY(BlueprintReadOnly, Category = "Session")
    int32 PingInMs = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruCreateSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBaruFindSessionsComplete, const TArray<FBaruSessionSearchResultInfo>&, SearchResults, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruJoinSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruDestroySessionComplete, bool, bWasSuccessful);

UCLASS()
class BARUGAME_API UBaruSessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UBaruSessionSubsystem();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "BARU|Session")
    void CreateSession(int32 NumPublicConnections = 5, bool bIsLANMatch = false, const FString& ServerName = TEXT("BARU Room"), TSoftObjectPtr<UWorld> OverrideLobbyLevel = nullptr);

    UFUNCTION(BlueprintCallable, Category = "BARU|Session")
    void FindSessions(int32 MaxSearchResults = 50, bool bIsLANMatch = false);

    UFUNCTION(BlueprintCallable, Category = "BARU|Session")
    void JoinSessionByIndex(int32 SessionIndex);

    UFUNCTION(BlueprintCallable, Category = "BARU|Session")
    void DestroySession(bool bReturnToMainMenu = false);

    UFUNCTION(BlueprintCallable, Category = "BARU|Session")
    void OpenFriendInviteUI();

    UFUNCTION(BlueprintPure, Category = "BARU|Session")
    bool IsSessionActive() const;

public:
    UPROPERTY(BlueprintAssignable, Category = "BARU|Session|Delegates")
    FOnBaruCreateSessionComplete OnCreateSessionCompleteEvent;

    UPROPERTY(BlueprintAssignable, Category = "BARU|Session|Delegates")
    FOnBaruFindSessionsComplete OnFindSessionsCompleteEvent;

    UPROPERTY(BlueprintAssignable, Category = "BARU|Session|Delegates")
    FOnBaruJoinSessionComplete OnJoinSessionCompleteEvent;

    UPROPERTY(BlueprintAssignable, Category = "BARU|Session|Delegates")
    FOnBaruDestroySessionComplete OnDestroySessionCompleteEvent;
    
protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BARU|Session|Maps")
    TSoftObjectPtr<UWorld> DefaultMainLobbyLevel;
    
private:
    void OpenLobbyLevelAsListenServer(const TSoftObjectPtr<UWorld>& LevelToOpen);

    void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
    // [수정] CreateSession 이후 세션을 InProgress 상태로 확정 짓기 위한 StartSession 콜백 추가
    void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);
    void OnFindSessionsComplete(bool bWasSuccessful);
    void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
    void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

    // [수정 - 신규 추가] 스팀 오버레이(Shift+Tab) 초대 수락 콜백 함수
    void OnSessionUserInviteAccepted(
        const bool bWasSuccessful,
        const int32 ControllerId,
        FUniqueNetIdPtr UserId,
        const FOnlineSessionSearchResult& InviteResult
    );

    // [수정 - 신규 추가] 인덱스 기반 참가 및 친구 초대 수락 참가를 일원화하는 공용 참가 헬퍼
    bool JoinSessionInternal(const FOnlineSessionSearchResult& SearchResult);

    IOnlineSessionPtr GetSessionInterface() const;

    void HandleNetworkFailure(
        UWorld* World, 
        UNetDriver* NetDriver, 
        ENetworkFailure::Type FailureType, 
        const FString& ErrorString
    );

private:
    FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
    FDelegateHandle CreateSessionCompleteDelegateHandle;

    // [수정] StartSession 완료 처리를 위한 델리게이트 및 핸들 추가
    FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;
    FDelegateHandle StartSessionCompleteDelegateHandle;

    FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
    FDelegateHandle FindSessionsCompleteDelegateHandle;

    FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
    FDelegateHandle JoinSessionCompleteDelegateHandle;

    FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
    FDelegateHandle DestroySessionCompleteDelegateHandle;

    // [수정 - 신규 추가] 스팀 오버레이 초대 수락 전용 델리게이트 및 안전장치 핸들
    FOnSessionUserInviteAcceptedDelegate OnSessionUserInviteAcceptedDelegate;
    FDelegateHandle OnSessionUserInviteAcceptedDelegateHandle;

    FDelegateHandle NetworkFailureDelegateHandle;

    TSharedPtr<FOnlineSessionSearch> LastSessionSearch;
    TSharedPtr<FOnlineSessionSettings> LastSessionSettings;

    TSoftObjectPtr<UWorld> StoredLobbyLevel;
    FString StoredServerName;
    int32 StoredNumConnections = 5;
    bool bStoredIsLANMatch = false;

    bool bCreateSessionAfterDestroy = false;
    bool bPendingReturnToMainMenu = false;
};