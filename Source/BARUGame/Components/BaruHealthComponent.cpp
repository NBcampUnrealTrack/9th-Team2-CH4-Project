#include "BaruHealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "NativeGameplayTags.h"

// 로그 매크로 포함 
// #include "BaruLog.h" 

// UI 및 외부 시스템 전달용 글로벌 태그 선언 
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Message_Combat_HealthChanged, "Message.Combat.HealthChanged");

UBaruHealthComponent::UBaruHealthComponent()
{
	// Tick 최소화 원칙: 체력 관리는 이벤트 기반이므로 Tick 완전 비활성화
	PrimaryComponentTick.bCanEverTick = false;

	MaxHealth = 100.0f;
	Health = 0.0f;
	bIsDead = false;

	// 생성자 내부에서 GetWorld(), Subsystem 호출 금지 (CDO 침범 방지)
}

void UBaruHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 상태 복제 등록
	DOREPLIFETIME(UBaruHealthComponent, MaxHealth);
	DOREPLIFETIME(UBaruHealthComponent, Health);
	DOREPLIFETIME(UBaruHealthComponent, bIsDead);
}

void UBaruHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// 서버에서만 초기 체력 설정 (데이터 주권)
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		Health = MaxHealth;
	}
}

void UBaruHealthComponent::ApplyDamage(float DamageAmount, AActor* Instigator)
{
	//  데이터 주권자: 체력 및 피격 판정은 오직 서버에서만 확정
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		// BARU_NET_LOG(GetWorld(), LogBaruCombat, Warning, TEXT("ApplyDamage called on Client. Ignoring request for %s"), *GetNameSafe(GetOwner()));
		return;
	}

	if (bIsDead || DamageAmount <= 0.0f)
	{
		return;
	}

	const float OldHealth = Health;
	Health = FMath::Clamp(Health - DamageAmount, 0.0f, MaxHealth);

	// 서버 측 자체 브로드캐스트 (서버 로컬 연출 및 AI 판단용)
	BroadcastHealthChanged(OldHealth, Health, Instigator);

	// 사망 판정
	if (Health <= 0.0f && !bIsDead)
	{
		bIsDead = true;
		OnDeath.Broadcast(Instigator);
		
		// BARU_NET_LOG(GetWorld(), LogBaruCombat, Log, TEXT("%s is Dead. Instigator: %s"), *GetNameSafe(GetOwner()), *GetNameSafe(Instigator));
	}
}

void UBaruHealthComponent::ApplyHeal(float HealAmount, AActor* Instigator)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (bIsDead || HealAmount <= 0.0f)
	{
		return;
	}

	const float OldHealth = Health;
	Health = FMath::Clamp(Health + HealAmount, 0.0f, MaxHealth);

	BroadcastHealthChanged(OldHealth, Health, Instigator);
}

void UBaruHealthComponent::OnRep_Health(float OldHealth)
{
	// 클라이언트는 OnRep에서 로컬 연출 및 UI 갱신만 수행
	BroadcastHealthChanged(OldHealth, Health, nullptr);
}

void UBaruHealthComponent::OnRep_IsDead()
{
	// 클라이언트 측 사망 연출 실행
	if (bIsDead)
	{
		OnDeath.Broadcast(nullptr);
	}
}

void UBaruHealthComponent::BroadcastHealthChanged(float OldHealth, float NewHealth, AActor* Instigator)
{
	// 1. 해당 액터(Owner) 로컬 델리게이트 발송 (애니메이션, 이펙트 재생용)
	OnHealthChanged.Broadcast(this, OldHealth, NewHealth, Instigator);

	// 2. UI 및 타 시스템 디커플링용 Gameplay Message 발송 
	if (UWorld* World = GetWorld())
	{
		FBaruHealthChangedMessage Message;
		Message.OwnerActor = GetOwner();
		Message.CurrentHealth = NewHealth;
		Message.MaxHealth = MaxHealth;
		
	}
}