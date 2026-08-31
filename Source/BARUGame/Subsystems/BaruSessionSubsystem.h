#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "UObject/SoftObjectPtr.h"
#include "BaruSessionSubsystem.generated.h"

class UWorld;

// BARU 게임만 선별하기 위한 식별 키
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

    // 방 만들기 (로비 선진입 상태에서는 맵 재로드 없이 세션만 활성화)
    UFUNCTION(BlueprintCallable, Category = "BARU|Session")
    void CreateSession(int32 NumPublicConnections = 5, bool bIsLANMatch = false, const FString& ServerName = TEXT("BARU Room"), TSoftObjectPtr<UWorld> OverrideLobbyLevel = nullptr);

    // 방 찾기 (Spacewar 중 MATCH_KEY 필터링)
    UFUNCTION(BlueprintCallable, Category = "BARU|Session")
    void FindSessions(int32 MaxSearchResults = 50, bool bIsLANMatch = false);

    // 방 참가
    UFUNCTION(BlueprintCallable, Category = "BARU|Session")
    void JoinSessionByIndex(int32 SessionIndex);

    // 세션 파괴 (bReturnToMainMenu가 false이면 로비에 잔류하여 솔로 플레이 모드 유지)
    UFUNCTION(BlueprintCallable, Category = "BARU|Session")
    void DestroySession(bool bReturnToMainMenu = false);

    // 스팀 오버레이 친구 초대 창 호출
    UFUNCTION(BlueprintCallable, Category = "BARU|Session")
    void OpenFriendInviteUI();

    UFUNCTION(BlueprintPure, Category = "BARU|Session")
    bool IsSessionActive() const;

public:
    // UI 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "BARU|Session|Delegates")
    FOnBaruCreateSessionComplete OnCreateSessionCompleteEvent;

    UPROPERTY(BlueprintAssignable, Category = "BARU|Session|Delegates")
    FOnBaruFindSessionsComplete OnFindSessionsCompleteEvent;

    UPROPERTY(BlueprintAssignable, Category = "BARU|Session|Delegates")
    FOnBaruJoinSessionComplete OnJoinSessionCompleteEvent;

    UPROPERTY(BlueprintAssignable, Category = "BARU|Session|Delegates")
    FOnBaruDestroySessionComplete OnDestroySessionCompleteEvent;
    
protected:
    /** 기본 트럭 UI 및 방 만들기용 로비 레벨 에셋 레퍼런스 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "BARU|Session|Maps")
    TSoftObjectPtr<UWorld> DefaultMainLobbyLevel;
    
private:
    void OpenLobbyLevelAsListenServer(const TSoftObjectPtr<UWorld>& LevelToOpen);

    void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
    void OnFindSessionsComplete(bool bWasSuccessful);
    void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
    void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

    IOnlineSessionPtr GetSessionInterface() const;

private:
    // [Handlers & pointer]
    FOnCreateSessionCompleteDelegate CreateSessionCompleteDelegate;
    FDelegateHandle CreateSessionCompleteDelegateHandle;

    FOnFindSessionsCompleteDelegate FindSessionsCompleteDelegate;
    FDelegateHandle FindSessionsCompleteDelegateHandle;

    FOnJoinSessionCompleteDelegate JoinSessionCompleteDelegate;
    FDelegateHandle JoinSessionCompleteDelegateHandle;

    FOnDestroySessionCompleteDelegate DestroySessionCompleteDelegate;
    FDelegateHandle DestroySessionCompleteDelegateHandle;

    TSharedPtr<FOnlineSessionSearch> LastSessionSearch;
    TSharedPtr<FOnlineSessionSettings> LastSessionSettings;

    TSoftObjectPtr<UWorld> StoredLobbyLevel;
    FString StoredServerName;
    bool bCreateSessionAfterDestroy = false;
    bool bPendingReturnToMainMenu = false;
};