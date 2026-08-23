#include "Subsystems/BaruSessionSubsystem.h"
#include "BaruLog.h"

UBaruSessionSubsystem::UBaruSessionSubsystem()
{
}

void UBaruSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	BARU_LOG(LogBaruSession, Log, TEXT("BaruSessionSubsystem Initialized."));
}

void UBaruSessionSubsystem::Deinitialize()
{
	BARU_LOG(LogBaruSession, Log, TEXT("BaruSessionSubsystem Deinitialized."));
	Super::Deinitialize();
}

void UBaruSessionSubsystem::CreateSession(int32 NumPublicConnections, bool bIsLANMatch)
{
	BARU_LOG(LogBaruSession, Log, TEXT("CreateSession called (Connections: %d, LAN: %d)"), NumPublicConnections, bIsLANMatch);
    
	// TODO: OnlineSubsystem OSS 세션 생성 로직 구현 예정
	OnCreateSessionCompleteEvent.Broadcast(true);
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
    
	// TODO: OnlineSubsystem OSS 세션 참가 및 ClientTravel 처리 예정
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