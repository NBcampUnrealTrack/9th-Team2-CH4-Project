#include "Player/BaruPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/BaruCoreAttributeSet.h"
#include "BaruLog.h"

ABaruPlayerState::ABaruPlayerState()
{
	PrimaryActorTick.bCanEverTick = false;
	
	bReplicates = true;
	SetNetUpdateFrequency(10.0f);

	// GAS Components
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// AttributeSet
	AttributeSet = CreateDefaultSubobject<UBaruCoreAttributeSet>(TEXT("AttributeSet"));
}

void ABaruPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaruPlayerState, bIsReady);
	DOREPLIFETIME(ABaruPlayerState, bIsDBNO);
	DOREPLIFETIME(ABaruPlayerState, Sanity);
}

UAbilitySystemComponent* ABaruPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// Server RPC
bool ABaruPlayerState::Server_SetReadyStatus_Validate(bool bNewReady)
{
	return true;
}

void ABaruPlayerState::Server_SetReadyStatus_Implementation(bool bNewReady)
{
	if (bIsReady == bNewReady)
	{
		return;
	}

	bIsReady = bNewReady;
	BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Player Ready Status Changed: %d"), bIsReady);
	
	OnReadyStatusChanged.Broadcast(bIsReady);
}

bool ABaruPlayerState::Server_UpdateNickname_Validate(const FString& NewNickname)
{
	return !NewNickname.TrimStartAndEnd().IsEmpty() && NewNickname.Len() <= 24;
}

void ABaruPlayerState::Server_UpdateNickname_Implementation(const FString& NewNickname)
{
	const FString TrimmedName = NewNickname.TrimStartAndEnd();
	SetPlayerName(TrimmedName);
	BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Player Nickname Updated to: %s"), *TrimmedName);
}

// [Server Only] 상태 변경 함수
void ABaruPlayerState::SetDBNOState(bool bNewDBNO)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bIsDBNO != bNewDBNO)
	{
		bIsDBNO = bNewDBNO;
		OnRep_IsDBNO(); // Listen Server 환경 및 서버 로직 즉시 갱신
	}
}

void ABaruPlayerState::SetSanityValue(float NewSanity)
{
	if (!HasAuthority())
	{
		return;
	}

	Sanity = FMath::Clamp(NewSanity, 0.0f, 100.0f);
	OnRep_Sanity();
}

// [Client] OnRep
void ABaruPlayerState::OnRep_IsReady()
{
	BARU_NET_LOG(this, LogBaruSession, Verbose, TEXT("OnRep_IsReady: %d"), bIsReady);
	OnReadyStatusChanged.Broadcast(bIsReady);
}

void ABaruPlayerState::OnRep_IsDBNO()
{
	BARU_NET_LOG(this, LogBaruCombat, Log, TEXT("OnRep_IsDBNO: %d"), bIsDBNO);
	OnDBNOStatusChanged.Broadcast(bIsDBNO);
}

void ABaruPlayerState::OnRep_Sanity()
{
	BARU_NET_LOG(this, LogBaruSanity, Verbose, TEXT("OnRep_Sanity: %.1f"), Sanity);
	OnSanityChanged.Broadcast(Sanity);
}