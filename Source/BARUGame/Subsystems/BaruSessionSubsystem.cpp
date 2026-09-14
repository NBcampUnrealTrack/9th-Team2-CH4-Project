#include "Subsystems/BaruSessionSubsystem.h"
#include "Subsystems/BaruSaveGameSubsystem.h"
#include "Player/BaruPlayerState.h"
#include "Gameplay/Inventory/BaruInventoryComponent.h"
#include "Gameplay/Items/BaruItemInstance.h"
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
#include "Misc/PackageName.h"
#include "BaruLog.h"
#include "Engine/GameInstance.h"

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
       NetworkFailureDelegateHandle = GEngine->OnNetworkFailure().AddUObject(
          this, 
          &UBaruSessionSubsystem::HandleNetworkFailure
       );
    }

    if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
    {
       OnSessionUserInviteAcceptedDelegateHandle = SessionInterface->AddOnSessionUserInviteAcceptedDelegate_Handle(OnSessionUserInviteAcceptedDelegate);
       BARU_LOG(LogBaruSession, Log, TEXT("Steam Overlay Invite Listener registered."));
    }
}

void UBaruSessionSubsystem::HandleNetworkFailure(
    UWorld* World, 
    UNetDriver* NetDriver, 
    ENetworkFailure::Type FailureType, 
    const FString& ErrorString)
{
    BARU_LOG(LogBaruSession, Warning, TEXT("Network Failure (%d): %s. Returning to MainMenuLevel."), 
       static_cast<int32>(FailureType), *ErrorString);
    
    if (World)
    {
       if (APlayerController* LocalPC = World->GetFirstPlayerController())
       {
          if (ABaruPlayerState* PS = LocalPC->GetPlayerState<ABaruPlayerState>())
          {
             if (UGameInstance* GI = GetGameInstance())
             {
                if (UBaruSaveGameSubsystem* SaveSubsystem = GI->GetSubsystem<UBaruSaveGameSubsystem>())
                {
                   int32 SalvagedGold = 0;
                   if (ABaruGameState* GS = World->GetGameState<ABaruGameState>())
                   {
                      SalvagedGold = FMath::RoundToInt(GS->GetTeamScrapValue() * 0.5f);
                   }

                   const FString PlayerName = PS->GetPlayerName().IsEmpty() ? TEXT("Operative") : PS->GetPlayerName();
                   SaveSubsystem->RecordRaidResult(PlayerName, SalvagedGold, /*bSurvived=*/false);
                   BARU_LOG(LogBaruSession, Log, TEXT("[EMERGENCY_SAVE] Salvaged %d Gold saved locally for %s"), SalvagedGold, *PlayerName);
                }
             }
          }
       }
    }
    
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

    IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
    if (!Identity.IsValid() || Identity->GetLoginStatus(0) != ELoginStatus::LoggedIn)
    {
        BARU_LOG(LogBaruSession, Warning, TEXT("GetSessionInterface: Local user 0 is not logged into Steam."));
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

    // 이미 열린 세션이 있는 경우 정리 후 재생성 플래그 설정
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

    // BARU 게임 Room 식별을 위한 메타데이터 등록
    LastSessionSettings->Set(BaruMatchmakingConstants::SETTING_MATCH_KEY, BaruMatchmakingConstants::BARU_MATCH_KEY_VALUE, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    LastSessionSettings->Set(BaruMatchmakingConstants::SETTING_SERVER_NAME, ServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    LastSessionSettings->Set(BaruMatchmakingConstants::SETTING_MAP_NAME, MapAssetPath, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    LastSessionSettings->Set(BaruMatchmakingConstants::SETTING_HOST_NAME, HostPlayerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

    const ULocalPlayer* LocalPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;
    FUniqueNetIdRepl NetId = LocalPlayer ? LocalPlayer->GetPreferredUniqueNetId() : FUniqueNetIdRepl();

    BARU_LOG(LogBaruSession, Log, TEXT("CreateSession: Creating Steam Lobby Session (Connections=%d, ServerName='%s')"), NumPublicConnections, *ServerName);

    if (!NetId.IsValid() || !NetId.GetUniqueNetId().IsValid() || !SessionInterface->CreateSession(*NetId.GetUniqueNetId(), NAME_GameSession, *LastSessionSettings))
    {
       SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
       BARU_LOG(LogBaruSession, Error, TEXT("CreateSession failed immediately on execution."));
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

    BARU_LOG(LogBaruSession, Log, TEXT("CreateSession complete. Advancing lifecycle to StartSession..."));

    StartSessionCompleteDelegateHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(StartSessionCompleteDelegate);
    if (!SessionInterface->StartSession(NAME_GameSession))
    {
        BARU_LOG(LogBaruSession, Warning, TEXT("StartSession returned false immediately. Proceeding directly to map setup."));
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

    BARU_LOG(LogBaruSession, Log, TEXT("OnStartSessionComplete: Success=%d. Verifying host listen state..."), bWasSuccessful);

    const bool bAlreadyListenServer = (GetWorld() && GetWorld()->GetNetMode() == NM_ListenServer);
    const FString CurrentMapName = GetWorld() ? GetWorld()->GetMapName() : TEXT("");
    const FString TargetMapName = StoredLobbyLevel.GetAssetName();

    if (!bAlreadyListenServer || !CurrentMapName.Contains(TargetMapName))
    {
        BARU_LOG(LogBaruSession, Log, TEXT("Host is not yet in ListenServer mode. Opening level with ?listen: %s"), *TargetMapName);
        OpenLobbyLevelAsListenServer(StoredLobbyLevel);
    }
    else
    {
        BARU_LOG(LogBaruSession, Log, TEXT("Host is already running ListenServer in lobby map. Skipping map reload."));
    }

    OnCreateSessionCompleteEvent.Broadcast(true);
}

void UBaruSessionSubsystem::FindSessions(int32 MaxSearchResults, bool bIsLANMatch)
{
    IOnlineSessionPtr SessionInterface = GetSessionInterface();
    const ULocalPlayer* LocalPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;
    FUniqueNetIdRepl NetId = LocalPlayer ? LocalPlayer->GetPreferredUniqueNetId() : FUniqueNetIdRepl();

    BARU_LOG(LogBaruSession, Log, TEXT("FindSessions Requested. MaxResults=%d, IsLAN=%d"), MaxSearchResults, bIsLANMatch);

    if (!SessionInterface.IsValid() || !NetId.IsValid() || !NetId.GetUniqueNetId().IsValid())
    {
       BARU_LOG(LogBaruSession, Error, TEXT("FindSessions Failed: Invalid SessionInterface or NetId."));
       OnFindSessionsCompleteEvent.Broadcast(TArray<FBaruSessionSearchResultInfo>(), false);
       return;
    }

    FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegate);

    LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
    LastSessionSearch->MaxSearchResults = FMath::Clamp(MaxSearchResults, 50, 100);
    LastSessionSearch->bIsLanQuery = bIsLANMatch;
    LastSessionSearch->TimeoutInSeconds = 10.0f;

    LastSessionSearch->QuerySettings.Set(FName(TEXT("LOBBIESSEARCH")), true, EOnlineComparisonOp::Equals);
    LastSessionSearch->QuerySettings.Set(FName(TEXT("PRESENCESEARCH")), true, EOnlineComparisonOp::Equals);
    
    if (!bIsLANMatch)
    {
       LastSessionSearch->QuerySettings.Set(
          BaruMatchmakingConstants::SETTING_MATCH_KEY, 
          BaruMatchmakingConstants::BARU_MATCH_KEY_VALUE, 
          EOnlineComparisonOp::Equals
       );
    }
    
    BARU_LOG(LogBaruSession, Log, TEXT("FindSessions: Executing Search via Steam Matchmaking Lobbies..."));

    if (!SessionInterface->FindSessions(*NetId.GetUniqueNetId(), LastSessionSearch.ToSharedRef()))
    {
       SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
       BARU_LOG(LogBaruSession, Error, TEXT("FindSessions request was rejected by Online Subsystem."));
       OnFindSessionsCompleteEvent.Broadcast(TArray<FBaruSessionSearchResultInfo>(), false);
    }
}

void UBaruSessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
    if (IOnlineSessionPtr SessionInterface = GetSessionInterface())
    {
       SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
    }

    TArray<FBaruSessionSearchResultInfo> FilteredResults;
    const int32 RawResultCount = LastSessionSearch.IsValid() ? LastSessionSearch->SearchResults.Num() : 0;

    BARU_LOG(LogBaruSession, Log, TEXT("OnFindSessionsComplete: Success=%d, Total Raw Results Found=%d"), bWasSuccessful, RawResultCount);
    
    if (bWasSuccessful && LastSessionSearch.IsValid())
    {
       for (int32 i = 0; i < LastSessionSearch->SearchResults.Num(); ++i)
       {
          const FOnlineSessionSearchResult& SearchResult = LastSessionSearch->SearchResults[i];

          FString MatchKey;
          SearchResult.Session.SessionSettings.Get(BaruMatchmakingConstants::SETTING_MATCH_KEY, MatchKey);

          FString FoundServerName;
          SearchResult.Session.SessionSettings.Get(BaruMatchmakingConstants::SETTING_SERVER_NAME, FoundServerName);
          
          BARU_LOG(LogBaruSession, Verbose, TEXT(" - Raw Room [%d]: ID=%s, Presence=%d, MatchKey='%s', ServerName='%s'"),
             i, *SearchResult.GetSessionIdStr(), SearchResult.Session.SessionSettings.bUsesPresence, *MatchKey, *FoundServerName);
          
          const bool bHasMatchKey = (MatchKey == BaruMatchmakingConstants::BARU_MATCH_KEY_VALUE);
          const bool bHasOpenSlots = (SearchResult.Session.NumOpenPublicConnections > 0);

          if (!SearchResult.IsValid() || !bHasMatchKey || !bHasOpenSlots)
          {
             continue;
          }

          FBaruSessionSearchResultInfo Info;
          Info.SessionIndex = i;
          Info.CurrentPlayers = SearchResult.Session.SessionSettings.NumPublicConnections - SearchResult.Session.NumOpenPublicConnections;
          Info.MaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
          Info.PingInMs = SearchResult.PingInMs;
          Info.ServerName = FoundServerName;

          SearchResult.Session.SessionSettings.Get(BaruMatchmakingConstants::SETTING_MAP_NAME, Info.SelectedMapName);
          SearchResult.Session.SessionSettings.Get(BaruMatchmakingConstants::SETTING_HOST_NAME, Info.HostPlayerName);

          FilteredResults.Add(Info);
       }
    }

    BARU_LOG(LogBaruSession, Log, TEXT("FindSessions Finished. Filtered BARU Rooms: %d / %d"), FilteredResults.Num(), RawResultCount);
    OnFindSessionsCompleteEvent.Broadcast(FilteredResults, bWasSuccessful);
}

bool UBaruSessionSubsystem::JoinSessionInternal(const FOnlineSessionSearchResult& SearchResult)
{
    IOnlineSessionPtr SessionInterface = GetSessionInterface();
    const ULocalPlayer* LocalPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;
    FUniqueNetIdRepl NetId = LocalPlayer ? LocalPlayer->GetPreferredUniqueNetId() : FUniqueNetIdRepl();

    if (!SessionInterface.IsValid() || !NetId.IsValid() || !NetId.GetUniqueNetId().IsValid() || !SearchResult.IsValid())
    {
       BARU_LOG(LogBaruSession, Error, TEXT("JoinSessionInternal Failed: Invalid SessionInterface, NetId, or SearchResult."));
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
       BARU_LOG(LogBaruSession, Error, TEXT("JoinSession returned false immediately."));
       OnJoinSessionCompleteEvent.Broadcast(false);
       return false;
    }

    return true;
}

void UBaruSessionSubsystem::JoinSessionByIndex(int32 SessionIndex)
{
    if (!LastSessionSearch.IsValid() || !LastSessionSearch->SearchResults.IsValidIndex(SessionIndex))
    {
       BARU_LOG(LogBaruSession, Error, TEXT("JoinSessionByIndex Failed: Invalid LastSessionSearch or Index %d."), SessionIndex);
       OnJoinSessionCompleteEvent.Broadcast(false);
       return;
    }

    JoinSessionInternal(LastSessionSearch->SearchResults[SessionIndex]);
}

void UBaruSessionSubsystem::OnSessionUserInviteAccepted(
    const bool bWasSuccessful,
    const int32 ControllerId,
    FUniqueNetIdPtr UserId,
    const FOnlineSessionSearchResult& InviteResult)
{
    BARU_LOG(LogBaruSession, Log, TEXT("Steam Overlay Invite Accepted! Success=%d, SessionId=%s"), 
        bWasSuccessful, InviteResult.IsValid() ? *InviteResult.GetSessionIdStr() : TEXT("INVALID"));

    if (!bWasSuccessful || !InviteResult.IsValid())
    {
        BARU_LOG(LogBaruSession, Warning, TEXT("OnSessionUserInviteAccepted: Invite acceptance failed or result is invalid."));
        return;
    }

    IOnlineSessionPtr SessionInterface = GetSessionInterface();
    if (!SessionInterface.IsValid())
    {
        return;
    }

    if (SessionInterface->GetNamedSession(NAME_GameSession))
    {
        BARU_LOG(LogBaruSession, Log, TEXT("Pre-existing session found during invite join. Destroying previous session..."));
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
          else
          {
             BARU_LOG(LogBaruSession, Error, TEXT("GetResolvedConnectString failed to retrieve Steam connect address."));
          }
       }
       else
       {
          BARU_LOG(LogBaruSession, Error, TEXT("OnJoinSessionComplete failed with Result Code: %d"), static_cast<int32>(Result));
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
       BARU_LOG(LogBaruSession, Log, TEXT("Session Destroyed. Returning to MainMenuLevel."));
       UGameplayStatics::OpenLevel(GetWorld(), TEXT("MainMenuLevel"));
    }
    else
    {
       BARU_LOG(LogBaruSession, Log, TEXT("Session Destroyed. Staying in Lobby (Offline Solo Mode)."));
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