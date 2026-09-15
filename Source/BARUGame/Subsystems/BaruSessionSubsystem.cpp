#include "Subsystems/BaruSessionSubsystem.h"
#include "Subsystems/BaruSaveGameSubsystem.h"
#include "Player/BaruPlayerState.h"
#include "Core/BaruGameState.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Online/OnlineSessionNames.h"
#include "Kismet/GameplayStatics.h"
#include "BaruLog.h"
#include "Engine/GameInstance.h"
#include "Async/Async.h"

UBaruSessionSubsystem::UBaruSessionSubsystem()
    : CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete))
    , StartSessionCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete))
    , FindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete))
    , JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete))
    , DestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete))
    , OnSessionUserInviteAcceptedDelegate(FOnSessionUserInviteAcceptedDelegate::CreateUObject(this, &ThisClass::OnSessionUserInviteAccepted))
{
    DefaultMainLobbyLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/BARUGame/Maps/MainLobbyLevel.MainLobbyLevel")));
}

void UBaruSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    BARU_LOG(LogBaruSession, Log, TEXT("BaruSessionSubsystem Initialized. Default Lobby Map: %s"), *DefaultMainLobbyLevel.ToString());
    
    if (GEngine)
    {
       NetworkFailureDelegateHandle = GEngine->OnNetworkFailure().AddUObject(this, &UBaruSessionSubsystem::HandleNetworkFailure);
    }

    if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
    {
       OnSessionUserInviteAcceptedDelegateHandle = SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(OnSessionUserInviteAcceptedDelegate);
       BARU_LOG(LogBaruSession, Log, TEXT("Steam Overlay Invite Listener registered."));
    }
}

void UBaruSessionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
    BARU_LOG(LogBaruSession, Warning, TEXT("Network Failure (%d): %s. Returning to MainMenuLevel."), static_cast<int32>(FailureType), *ErrorString);
    
    DestroySession(false);

    if (UWorld* CurrentWorld = GetWorld())
    {
       UGameplayStatics::OpenLevel(CurrentWorld, TEXT("MainMenuLevel"));
    }
}

void UBaruSessionSubsystem::Deinitialize()
{
    BARU_LOG(LogBaruSession, Log, TEXT("BaruSessionSubsystem Deinitialized."));

    if (GEngine && NetworkFailureDelegateHandle.IsValid())
    {
       GEngine->OnNetworkFailure().Remove(NetworkFailureDelegateHandle);
       NetworkFailureDelegateHandle.Reset();
    }

    if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
    {
       SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
       SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
       SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
       SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
       SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
       SessionInterface->ClearOnSessionUserInviteAcceptedDelegate_Handle(OnSessionUserInviteAcceptedDelegateHandle);
    }

    DestroySession(false);
    Super::Deinitialize();
}

IOnlineSessionPtr UBaruSessionSubsystem::GetSessionInterface() const
{
    IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
    if (!Subsystem)
    {
        BARU_LOG(LogBaruSession, Error, TEXT("GetSessionInterface: OnlineSubsystem is unavailable."));
        return nullptr;
    }

    return Subsystem->GetSessionInterface();
}

bool UBaruSessionSubsystem::IsSessionActive() const
{
    if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
    {
       return SessionInterface->GetNamedSession(NAME_GameSession) != nullptr;
    }
    return false;
}

void UBaruSessionSubsystem::CreateSession(int32 NumPublicConnections, bool bIsLANMatch, const FString& ServerName, TSoftObjectPtr<UWorld> OverrideLobbyLevel)
{
    StoredLobbyLevel = OverrideLobbyLevel.IsNull() ? DefaultMainLobbyLevel : OverrideLobbyLevel;
    StoredServerName = ServerName;
    StoredNumConnections = NumPublicConnections;
    bStoredIsLANMatch = bIsLANMatch;

    IOnlineSessionPtr SessionInterface = GetSessionInterface();

    if (!SessionInterface.IsValid())
    {
       BARU_LOG(LogBaruSession, Warning, TEXT("CreateSession: OnlineSubsystem unavailable. Fallback to Local Listen Server."));
       if (GetWorld() && GetWorld()->GetNetMode() != NM_ListenServer)
       {
          OpenLobbyLevelAsListenServer(StoredLobbyLevel);
       }
       OnCreateSessionCompleteEvent.Broadcast(true);
       return;
    }

    if (SessionInterface->GetNamedSession(NAME_GameSession) != nullptr)
    {
       bCreateSessionAfterDestroy = true;
       DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);
       SessionInterface->DestroySession(NAME_GameSession);
       return;
    }

    CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

    LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
    LastSessionSettings->bIsLANMatch = bIsLANMatch;
    LastSessionSettings->NumPublicConnections = NumPublicConnections;
    LastSessionSettings->bAllowJoinInProgress = true;
    LastSessionSettings->bAllowJoinViaPresence = true;
    LastSessionSettings->bAllowInvites = true;
    LastSessionSettings->bAllowJoinViaPresenceFriendsOnly = false;
    LastSessionSettings->bShouldAdvertise = true;
    LastSessionSettings->bUsesPresence = true;
    LastSessionSettings->bUseLobbiesIfAvailable = true;

    FString HostPlayerName = TEXT("Host");
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
       if (PC->PlayerState)
       {
          HostPlayerName = PC->PlayerState->GetPlayerName();
       }
    }
    
    const FString MapAssetPath = StoredLobbyLevel.GetAssetName();

    // 메타데이터 설정 (언리얼 OSS가 스팀 백엔드에 _s를 자동으로 붙여서 전송함)
    LastSessionSettings->Set(BaruMatchmakingConstants::SETTING_SERVER_NAME, ServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    LastSessionSettings->Set(BaruMatchmakingConstants::SETTING_MAP_NAME, MapAssetPath, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    LastSessionSettings->Set(BaruMatchmakingConstants::SETTING_HOST_NAME, HostPlayerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    LastSessionSettings->Set(BaruMatchmakingConstants::SETTING_PROJECT_ID, FString(UTF8_TO_TCHAR(BaruMatchmakingConstants::RAW_PROJECT_VALUE)), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

    const ULocalPlayer* LocalPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;
    FUniqueNetIdRepl NetId = LocalPlayer ? LocalPlayer->GetPreferredUniqueNetId() : FUniqueNetIdRepl();

    BARU_LOG(LogBaruSession, Log, TEXT("CreateSession: Creating Steam Lobby (Connections=%d, ServerName='%s')"), NumPublicConnections, *ServerName);

    if (!NetId.IsValid() || !NetId.GetUniqueNetId().IsValid() || !SessionInterface->CreateSession(*NetId.GetUniqueNetId(), NAME_GameSession, *LastSessionSettings))
    {
       SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
       BARU_LOG(LogBaruSession, Error, TEXT("CreateSession failed immediately."));
       OnCreateSessionCompleteEvent.Broadcast(false);
    }
}

void UBaruSessionSubsystem::OpenLobbyLevelAsListenServer(const TSoftObjectPtr<UWorld>& LevelToOpen)
{
    if (LevelToOpen.IsNull())
    {
       BARU_LOG(LogBaruSession, Error, TEXT("OpenLobbyLevelAsListenServer Failed: Lobby Level path is NULL."));
       return;
    }

    BARU_LOG(LogBaruSession, Log, TEXT("Opening Lobby Level as Listen Server: %s"), *LevelToOpen.ToString());
    UGameplayStatics::OpenLevelBySoftObjectPtr(GetWorld(), LevelToOpen, true, TEXT("listen"));
}

void UBaruSessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
    IOnlineSessionPtr SessionInterface = GetSessionInterface();
    if (SessionInterface.IsValid())
    {
       SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
    }

    if (!bWasSuccessful || !SessionInterface.IsValid())
    {
        BARU_LOG(LogBaruSession, Error, TEXT("OnCreateSessionComplete: Session creation failed."));
        OnCreateSessionCompleteEvent.Broadcast(false);
        return;
    }

    StartSessionCompleteDelegateHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegate);
    if (!SessionInterface->StartSession(NAME_GameSession))
    {
        SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
        OnStartSessionComplete(NAME_GameSession, false);
    }
}

void UBaruSessionSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
    if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
    {
        SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegateHandle);
    }

    const bool bAlreadyListenServer = (GetWorld() && GetWorld()->GetNetMode() == NM_ListenServer);
    const FString CurrentMapName = GetWorld() ? GetWorld()->GetMapName() : TEXT("");
    const FString TargetMapName = StoredLobbyLevel.GetAssetName();

    if (!bAlreadyListenServer || !CurrentMapName.Contains(TargetMapName))
    {
        OpenLobbyLevelAsListenServer(StoredLobbyLevel);
    }

    OnCreateSessionCompleteEvent.Broadcast(true);
}

void UBaruSessionSubsystem::FindSessions(int32 MaxSearchResults, bool bIsLANMatch)
{
    if (!SteamMatchmaking())
    {
       BARU_LOG(LogBaruSession, Error, TEXT("FindSessions: SteamMatchmaking is null!"));
       OnFindSessionsCompleteEvent.Broadcast(TArray<FBaruSessionSearchResultInfo>(), false);
       return;
    }

    FoundSteamLobbyIDs.Empty();
    BARU_LOG(LogBaruSession, Log, TEXT("FindSessions: Requesting Filtered Steam Lobby List..."));

    SteamMatchmaking()->AddRequestLobbyListDistanceFilter(k_ELobbyDistanceFilterWorldwide);
    SteamMatchmaking()->AddRequestLobbyListResultCountFilter(FMath::Clamp(MaxSearchResults, 20, 100));

    // [핵심] 스팀 서버에 실제 저장된 "BARU_PROJECT_ID_s" 키로 필터링
    SteamMatchmaking()->AddRequestLobbyListStringFilter(
        BaruMatchmakingConstants::RAW_PROJECT_KEY, 
        BaruMatchmakingConstants::RAW_PROJECT_VALUE, 
        k_ELobbyComparisonEqual
    );

    SteamAPICall_t hSteamAPICall = SteamMatchmaking()->RequestLobbyList();
    SteamLobbyMatchListCallResult.Set(hSteamAPICall, this, &UBaruSessionSubsystem::OnSteamLobbyMatchList);
}

void UBaruSessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
    // 네이티브 콜백인 OnSteamLobbyMatchList를 사용하므로 호환성용 빈 델리게이트로 유지
}

bool UBaruSessionSubsystem::JoinSessionInternal(const FOnlineSessionSearchResult& SearchResult)
{
    IOnlineSessionPtr SessionInterface = GetSessionInterface();
    const ULocalPlayer* LocalPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;
    FUniqueNetIdRepl NetId = LocalPlayer ? LocalPlayer->GetPreferredUniqueNetId() : FUniqueNetIdRepl();

    if (!SessionInterface.IsValid() || !NetId.IsValid() || !NetId.GetUniqueNetId().IsValid() || !SearchResult.IsValid())
    {
       OnJoinSessionCompleteEvent.Broadcast(false);
       return false;
    }

    JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

    FOnlineSessionSearchResult SessionToJoin = SearchResult;
    SessionToJoin.Session.SessionSettings.bUsesPresence = true;
    SessionToJoin.Session.SessionSettings.bUseLobbiesIfAvailable = true;

    BARU_LOG(LogBaruSession, Log, TEXT("Joining Steam Session: SessionId=%s"), *SessionToJoin.GetSessionIdStr());

    if (!SessionInterface->JoinSession(*NetId.GetUniqueNetId(), NAME_GameSession, SessionToJoin))
    {
       SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
       OnJoinSessionCompleteEvent.Broadcast(false);
       return false;
    }

    return true;
}

void UBaruSessionSubsystem::JoinSessionByIndex(int32 SessionIndex)
{
    if (!FoundSteamLobbyIDs.IsValidIndex(SessionIndex))
    {
       BARU_LOG(LogBaruSession, Warning, TEXT("JoinSessionByIndex: Invalid Session Index: %d"), SessionIndex);
       OnJoinSessionCompleteEvent.Broadcast(false);
       return;
    }

    CSteamID TargetLobbyID = FoundSteamLobbyIDs[SessionIndex];
    CSteamID HostSteamID = SteamMatchmaking()->GetLobbyOwner(TargetLobbyID);

    if (!HostSteamID.IsValid())
    {
       BARU_LOG(LogBaruSession, Error, TEXT("JoinSessionByIndex: Failed to get Lobby Owner SteamID."));
       OnJoinSessionCompleteEvent.Broadcast(false);
       return;
    }

    // SteamSockets P2P 접속 주소: steam.<HostSteam64ID>:7777
    FString ConnectString = FString::Printf(TEXT("steam.%llu:7777"), HostSteamID.ConvertToUint64());
    BARU_LOG(LogBaruSession, Log, TEXT("Joining Steam Lobby: ConnectString=%s"), *ConnectString);

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
       PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
       OnJoinSessionCompleteEvent.Broadcast(true);
    }
    else
    {
       OnJoinSessionCompleteEvent.Broadcast(false);
    }
}

void UBaruSessionSubsystem::OnSessionUserInviteAccepted(
    const bool bWasSuccessful,
    const int32 ControllerId,
    FUniqueNetIdPtr UserId,
    const FOnlineSessionSearchResult& InviteResult)
{
    if (!bWasSuccessful || !InviteResult.IsValid())
    {
        return;
    }

    IOnlineSessionPtr SessionInterface = GetSessionInterface();
    if (!SessionInterface.IsValid())
    {
        return;
    }

    if (SessionInterface->GetNamedSession(NAME_GameSession))
    {
        SessionInterface->DestroySession(NAME_GameSession);
    }

    JoinSessionInternal(InviteResult);
}

void UBaruSessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
    if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
    {
       SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

       if (Result == EOnJoinSessionCompleteResult::Success)
       {
          FString ConnectInfo;
          if (SessionInterface->GetResolvedConnectString(NAME_GameSession, ConnectInfo) && !ConnectInfo.IsEmpty())
          {
             if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
             {
                BARU_LOG(LogBaruSession, Log, TEXT("ClientTravel to Steam Session: %s"), *ConnectInfo);
                PC->ClientTravel(ConnectInfo, ETravelType::TRAVEL_Absolute);
                OnJoinSessionCompleteEvent.Broadcast(true);
                return;
             }
          }
       }
    }

    OnJoinSessionCompleteEvent.Broadcast(false);
}

void UBaruSessionSubsystem::DestroySession(bool bReturnToMainMenu)
{
    bPendingReturnToMainMenu = bReturnToMainMenu;

    IOnlineSessionPtr SessionInterface = GetSessionInterface();
    if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(NAME_GameSession) != nullptr)
    {
       DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegate);
       SessionInterface->DestroySession(NAME_GameSession);
    }
    else
    {
       if (bPendingReturnToMainMenu)
       {
          UGameplayStatics::OpenLevel(GetWorld(), TEXT("MainMenuLevel"));
       }
       OnDestroySessionCompleteEvent.Broadcast(true);
    }
}

void UBaruSessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
    if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
    {
       SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
    }

    if (bPendingReturnToMainMenu)
    {
       bPendingReturnToMainMenu = false;
       UGameplayStatics::OpenLevel(GetWorld(), TEXT("MainMenuLevel"));
    }

    OnDestroySessionCompleteEvent.Broadcast(bWasSuccessful);

    if (bCreateSessionAfterDestroy)
    {
       bCreateSessionAfterDestroy = false;
       CreateSession(StoredNumConnections, bStoredIsLANMatch, StoredServerName, StoredLobbyLevel);
    }
}

void UBaruSessionSubsystem::OpenFriendInviteUI()
{
    if (IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld()))
    {
       if (IOnlineExternalUIPtr ExternalUI = Subsystem->GetExternalUIInterface())
       {
          ExternalUI->ShowInviteUI(0, NAME_GameSession);
       }
    }
}

void UBaruSessionSubsystem::OnSteamLobbyMatchList(LobbyMatchList_t* pLobbyMatchList, bool bIOFailure)
{
    if (bIOFailure || !pLobbyMatchList)
    {
        BARU_LOG(LogBaruSession, Error, TEXT("OnSteamLobbyMatchList: IO Failure or null response."));
        AsyncTask(ENamedThreads::GameThread, [this]()
        {
            OnFindSessionsCompleteEvent.Broadcast(TArray<FBaruSessionSearchResultInfo>(), false);
        });
        return;
    }

    const uint32 LobbyCount = pLobbyMatchList->m_nLobbiesMatching;
    BARU_LOG(LogBaruSession, Log, TEXT("OnSteamLobbyMatchList: Found %d Steam Lobbies."), LobbyCount);

    TArray<FBaruSessionSearchResultInfo> ResultList;
    TArray<CSteamID> NewLobbyIDs;

    for (uint32 i = 0; i < LobbyCount; ++i)
    {
        CSteamID LobbyID = SteamMatchmaking()->GetLobbyByIndex(i);
        if (!LobbyID.IsValid())
        {
            continue;
        }

        // [2차 방어] 실제 저장된 "BARU_PROJECT_ID_s" 키 검증
        const char* ProjectId = SteamMatchmaking()->GetLobbyData(LobbyID, BaruMatchmakingConstants::RAW_PROJECT_KEY);
        if (!ProjectId || FCStringAnsi::Strcmp(ProjectId, BaruMatchmakingConstants::RAW_PROJECT_VALUE) != 0)
        {
            continue;
        }

        NewLobbyIDs.Add(LobbyID);

        FBaruSessionSearchResultInfo Info;
        Info.SessionIndex = NewLobbyIDs.Num() - 1;
        Info.CurrentPlayers = SteamMatchmaking()->GetNumLobbyMembers(LobbyID);
        Info.MaxPlayers = SteamMatchmaking()->GetLobbyMemberLimit(LobbyID);
        Info.PingInMs = 0;

        // [실제 저장된 키 반영] SERVER_NAME_s, MAP_NAME_s, HOST_NAME_s
        const char* ServerName = SteamMatchmaking()->GetLobbyData(LobbyID, BaruMatchmakingConstants::RAW_SERVER_NAME_KEY);
        const char* MapName = SteamMatchmaking()->GetLobbyData(LobbyID, BaruMatchmakingConstants::RAW_MAP_NAME_KEY);
        const char* HostName = SteamMatchmaking()->GetLobbyData(LobbyID, BaruMatchmakingConstants::RAW_HOST_NAME_KEY);

        Info.ServerName = (ServerName && FCStringAnsi::Strlen(ServerName) > 0) ? UTF8_TO_TCHAR(ServerName) : TEXT("Steam Lobby");
        Info.SelectedMapName = (MapName && FCStringAnsi::Strlen(MapName) > 0) ? UTF8_TO_TCHAR(MapName) : TEXT("MainLobbyLevel");
        Info.HostPlayerName = (HostName && FCStringAnsi::Strlen(HostName) > 0) ? UTF8_TO_TCHAR(HostName) : TEXT("Host");

        ResultList.Add(Info);
    }

    AsyncTask(ENamedThreads::GameThread, [this, ResultList = MoveTemp(ResultList), NewLobbyIDs = MoveTemp(NewLobbyIDs)]() mutable
    {
        FoundSteamLobbyIDs = MoveTemp(NewLobbyIDs);
        OnFindSessionsCompleteEvent.Broadcast(ResultList, true);
    });
}