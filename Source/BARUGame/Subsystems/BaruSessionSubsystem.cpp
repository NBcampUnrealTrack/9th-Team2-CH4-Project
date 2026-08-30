#include "Subsystems/BaruSessionSubsystem.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Kismet/GameplayStatics.h"
#include "BaruLog.h"

UBaruSessionSubsystem::UBaruSessionSubsystem()
{
	DefaultMainLobbyLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/BARUGame/Maps/MainLobbyLevel.MainLobbyLevel")));
}

void UBaruSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	BARU_LOG(LogBaruSession, Log, TEXT("BaruSessionSubsystem Initialized. Default Lobby Map: %s"), *DefaultMainLobbyLevel.ToString());
}

void UBaruSessionSubsystem::Deinitialize()
{
	BARU_LOG(LogBaruSession, Log, TEXT("BaruSessionSubsystem Deinitialized."));
	
	Super::Deinitialize();
}

void UBaruSessionSubsystem::CreateSession(int32 NumPublicConnections, bool bIsLANMatch, TSoftObjectPtr<UWorld> OverrideLobbyLevel)
{
	BARU_LOG(LogBaruSession, Log, TEXT("CreateSession (Listen Server) called (Connections: %d, LAN: %d)"), NumPublicConnections, bIsLANMatch);

	const TSoftObjectPtr<UWorld> TargetLobbyLevel = OverrideLobbyLevel.IsNull() ? DefaultMainLobbyLevel : OverrideLobbyLevel;

	// nullptr 방지
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	IOnlineSessionPtr SessionInterface = Subsystem ? Subsystem->GetSessionInterface() : nullptr;

	if (!SessionInterface.IsValid())
	{
		BARU_LOG(LogBaruSession, Warning, TEXT("CreateSession: OnlineSubsystem is not available. Opening lobby level directly in local Listen Server mode."));
		OpenLobbyLevelAsListenServer(TargetLobbyLevel);
		OnCreateSessionCompleteEvent.Broadcast(true);
		return;
	}

	const ULocalPlayer* LocalPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;
	if (!LocalPlayer || !LocalPlayer->GetPreferredUniqueNetId().IsValid())
	{
		BARU_LOG(LogBaruSession, Warning, TEXT("CreateSession: LocalPlayer NetId is invalid. Opening level directly."));
		OpenLobbyLevelAsListenServer(TargetLobbyLevel);
		OnCreateSessionCompleteEvent.Broadcast(true);
		return;
	}

	// TODO: OnlineSubsystem 비동기 세션 생성(CreateSession) 콜백 완료 시 OpenLobbyLevelAsListenServer 실행
	OpenLobbyLevelAsListenServer(TargetLobbyLevel);
	OnCreateSessionCompleteEvent.Broadcast(true);
}

void UBaruSessionSubsystem::OpenLobbyLevelAsListenServer(const TSoftObjectPtr<UWorld>& LevelToOpen)
{
	if (LevelToOpen.IsNull())
	{
		BARU_LOG(LogBaruSession, Error, TEXT("OpenLobbyLevelAsListenServer Failed: Lobby Level path is NULL."));
		return;
	}

	BARU_LOG(LogBaruSession, Log, TEXT("Opening Lobby Level as Listen Server: %s"), *LevelToOpen.ToString());

	// OpenLevelBySoftObjectPtr를 사용하여 지정된 로비 맵을 ?listen 옵션과 함께 오픈
	UGameplayStatics::OpenLevelBySoftObjectPtr(GetWorld(), LevelToOpen, true, TEXT("listen"));
}

void UBaruSessionSubsystem::FindSessions(int32 MaxSearchResults, bool bIsLANMatch)
{
	BARU_LOG(LogBaruSession, Log, TEXT("FindSessions called (MaxResults: %d, LAN: %d)"), MaxSearchResults, bIsLANMatch);
    
	// TODO: OnlineSubsystem OSS 세션 검색 로직 구현 예정
	OnFindSessionsCompleteEvent.Broadcast(true);
}

void UBaruSessionSubsystem::JoinSessionByIndex(int32 SessionIndex)
{
	BARU_LOG(LogBaruSession, Log, TEXT("JoinSessionByIndex called (Index: %d)"), SessionIndex);

	// 세션 접속 성공 시 해당 호스트 IP로 이동
	
	OnJoinSessionCompleteEvent.Broadcast(true);
}

void UBaruSessionSubsystem::DestroySession()
{
	BARU_LOG(LogBaruSession, Log, TEXT("DestroySession called."));
    
	// TODO: OnlineSubsystem OSS 세션 정리 로직 구현 예정
	OnDestroySessionCompleteEvent.Broadcast(true);
}

int32 UBaruSessionSubsystem::GetSearchResultsCount() const
{
	return 0;
}