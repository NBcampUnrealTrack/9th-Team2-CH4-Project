#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "UObject/SoftObjectPtr.h"
#include "Engine/EngineBaseTypes.h"
#include "steam/steam_api.h"
#include "BaruSessionSubsystem.generated.h"

class UWorld;
class UNetDriver;

namespace BaruMatchmakingConstants
{
    // 언리얼 OSS FOnlineSessionSettings에 넘기는 키 (언리얼 내부에서 _s 접미사를 자동 부착)
    const FName SETTING_SERVER_NAME = FName(TEXT("SERVER_NAME"));
    const FName SETTING_MAP_NAME = FName(TEXT("MAP_NAME"));
    const FName SETTING_HOST_NAME = FName(TEXT("HOST_NAME"));
    const FName SETTING_PROJECT_ID = FName(TEXT("BARU_PROJECT_ID"));

    // 스팀 네이티브 SDK(SteamMatchmaking)로 직접 쿼리/조회할 때 쓰는 실제 키 이름 (_s 반영)
    inline const char* RAW_PROJECT_KEY = "BARU_PROJECT_ID_s";
    inline const char* RAW_PROJECT_VALUE = "BARU_PROJECT_2026_V1";
    inline const char* RAW_SERVER_NAME_KEY = "SERVER_NAME_s";
    inline const char* RAW_MAP_NAME_KEY = "MAP_NAME_s";
    inline const char* RAW_HOST_NAME_KEY = "HOST_NAME_s";
}

USTRUCT(BlueprintType)
struct FBaruSessionSearchResultInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Session")
    int32 SessionIndex = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Session")
    FString ServerName = TEXT("BARU Room");

    UPROPERTY(BlueprintReadOnly, Category = "Session")
    FString HostPlayerName = TEXT("Host");

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
    void FindSessions(int32 MaxSearchResults = 100, bool bIsLANMatch = false);

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
    void OnStartSessionComplete(FName SessionName, bool bWasSuccessful);
    void OnFindSessionsComplete(bool bWasSuccessful);
    void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
    void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

    void OnSessionUserInviteAccepted(
        const bool bWasSuccessful,
        const int32 ControllerId,
        FUniqueNetIdPtr UserId,
        const FOnlineSessionSearchResult& InviteResult
    );

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

    FOnStartSessionCompleteDelegate StartSessionCompleteDelegate;
    FDelegateHandle StartSessionCompleteDelegateHandle;

    FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
    FDelegateHandle FindSessionsCompleteDelegateHandle;

    FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
    FDelegateHandle JoinSessionCompleteDelegateHandle;

    FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
    FDelegateHandle DestroySessionCompleteDelegateHandle;

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
    
private:
    CCallResult<UBaruSessionSubsystem, LobbyMatchList_t> SteamLobbyMatchListCallResult;
    void OnSteamLobbyMatchList(LobbyMatchList_t* pLobbyMatchList, bool bIOFailure);

    TArray<CSteamID> FoundSteamLobbyIDs;
};