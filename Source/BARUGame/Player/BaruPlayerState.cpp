#include "Player/BaruPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/BaruAttributeSet.h"
#include "BaruLog.h"
#include "Components/BaruHealthComponent.h"

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
	AttributeSet = CreateDefaultSubobject<UBaruAttributeSet>(TEXT("AttributeSet"));
	HealthComponent = CreateDefaultSubobject<UBaruHealthComponent>(TEXT("HealthComponent"));
}

void ABaruPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaruPlayerState, bIsReady);
	DOREPLIFETIME(ABaruPlayerState, bIsDBNO);
	DOREPLIFETIME(ABaruPlayerState, Sanity);
	
	DOREPLIFETIME(ABaruPlayerState, Health);
	DOREPLIFETIME(ABaruPlayerState, MaxHealth);
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

void ABaruPlayerState::SetHealthValue(float NewHealth)
{
	if (!HasAuthority())
	{
		return;
	}

	Health = FMath::Clamp(NewHealth, 0.0f, MaxHealth);
	OnRep_Health(); // 서버/리슨서버 호스트의 UI 및 델리게이트 즉시 방송
}

void ABaruPlayerState::SetMaxHealthValue(float NewMaxHealth)
{
	if (!HasAuthority())
	{
		return;
	}

	MaxHealth = FMath::Max(NewMaxHealth, 1.0f);
	OnRep_MaxHealth();
}

// OnRep
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

void ABaruPlayerState::OnRep_Health()
{
	BARU_NET_LOG(this, LogBaruCombat, Verbose, TEXT("OnRep_Health: %.1f / %.1f"), Health, MaxHealth);
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void ABaruPlayerState::OnRep_MaxHealth()
{
	OnHealthChanged.Broadcast(Health, MaxHealth);
}