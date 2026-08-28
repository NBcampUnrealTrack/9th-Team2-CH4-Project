#include "BaruHealthComponent.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerState.h" 
#include "GameFramework/Pawn.h"
#include "Player/BaruPlayerState.h"// ★[추가] 소유 PS → Pawn 조회
#include "GameplayEffectExtension.h"
#include "AbilitySystem/Attributes/BaruCoreAttributeSet.h"
#include "Interfaces/CombatInterface.h" 
#include "BaruLog.h"                                              // ★[추가] 로그


UBaruHealthComponent::UBaruHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    // [삭제] bIsDead = false; 초기화 삭제
}

void UBaruHealthComponent::OnUnregister()
{
    UninitializeFromAbilitySystem();
    Super::OnUnregister();
}

void UBaruHealthComponent::InitializeWithAbilitySystem(UAbilitySystemComponent* InASC)
{
    if (!InASC || AbilitySystemComponent == InASC)
    {
        return;
    }

    if (AbilitySystemComponent)
    {
        UninitializeFromAbilitySystem();
    }

    AbilitySystemComponent = InASC;

    // Attribute 변경 이벤트 바인딩
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBaruCoreAttributeSet::GetHealthAttribute())
        .AddUObject(this, &UBaruHealthComponent::HandleHealthChanged);

    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBaruCoreAttributeSet::GetMaxHealthAttribute())
        .AddUObject(this, &UBaruHealthComponent::HandleMaxHealthChanged);

    // 초기값 강제 갱신 (UI 세팅용)
    const float CurrentHealth = GetHealth();
    OnHealthChanged.Broadcast(this, CurrentHealth, CurrentHealth, nullptr);

    // ★[추가] 최대 체력도 초기 1회 통지
    const float CurrentMaxHealth = GetMaxHealth();
    OnMaxHealthChanged.Broadcast(CurrentMaxHealth, CurrentMaxHealth);
}

void UBaruHealthComponent::UninitializeFromAbilitySystem()
{
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBaruCoreAttributeSet::GetHealthAttribute()).RemoveAll(this);
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBaruCoreAttributeSet::GetMaxHealthAttribute()).RemoveAll(this);
        AbilitySystemComponent = nullptr;
    }
}

float UBaruHealthComponent::GetHealth() const
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->GetNumericAttribute(UBaruCoreAttributeSet::GetHealthAttribute());
    }
    return 0.0f;
}

float UBaruHealthComponent::GetMaxHealth() const
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent->GetNumericAttribute(UBaruCoreAttributeSet::GetMaxHealthAttribute());
    }
    return 0.0f;
}

// ★[추가] UI 프로그레스바용
float UBaruHealthComponent::GetHealthNormalized() const
{
    const float MaxHP = GetMaxHealth();
    return (MaxHP > 0.0f) ? FMath::Clamp(GetHealth() / MaxHP, 0.0f, 1.0f) : 0.0f;
}

// ★[수정] ★★ 중요 ★★
//   기존 코드는 GetOwner() 가 ICombatInterface 를 구현했는지 확인했는데,
//   이 컴포넌트의 Owner 는 ABaruPlayerState 이고 PlayerState 는 ICombatInterface 를
//   구현하지 않습니다. → 이 함수는 항상 false 를 반환하고 있었습니다.
//   실제 전투 주체는 PlayerState 가 소유한 Pawn(=ABaruCharacter) 이므로 그쪽을 봐야 합니다.
bool UBaruHealthComponent::IsDead() const
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return false;
    }

    // 1) 이 컴포넌트가 PlayerState 에 붙어 있는 경우(현재 구조) → 소유 Pawn 을 조회
    if (const APlayerState* OwnerPS = Cast<APlayerState>(Owner))
    {
        if (AActor* OwnerPawn = OwnerPS->GetPawn())
        {
            if (OwnerPawn->Implements<UCombatInterface>())
            {
                return ICombatInterface::Execute_IsDead(OwnerPawn);
            }
        }
        return false;
    }

    // 2) 나중에 이 컴포넌트를 Pawn 에 직접 붙이는 경우도 지원
    if (Owner->Implements<UCombatInterface>())
    {
        return ICombatInterface::Execute_IsDead(Owner);
    }

    return false;
}

// ★[추가] OnDeath 델리게이트를 실제로 터뜨리는 지점
void UBaruHealthComponent::NotifyDeath(AActor* Killer)
{
    BARU_LOG(LogBaruCombat, Log, TEXT("HealthComponent NotifyDeath. Killer: %s"),
        Killer ? *Killer->GetName() : TEXT("None"));

    // 주의: Killer 는 서버에서만 유효합니다.
    //       클라이언트에서는 사망 사실만 복제되고 킬러는 전달되지 않아 nullptr 입니다.
    //       킬 피드처럼 킬러 정보가 필요한 UI 는 별도 복제 설계가 필요합니다.
    OnDeath.Broadcast(Killer);
}

void UBaruHealthComponent::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
    AActor* Instigator = nullptr;

    if (ChangeData.GEModData)
    {
        const FGameplayEffectContextHandle& EffectContext = ChangeData.GEModData->EffectSpec.GetEffectContext();
        Instigator = EffectContext.GetInstigator();
    }

    // [수정] UI 및 이펙트 갱신용 브로드캐스트만 수행합니다.
    OnHealthChanged.Broadcast(this, ChangeData.OldValue, ChangeData.NewValue, Instigator);

    // [삭제] 사망 판정 로직 삭제 (BaruCoreAttributeSet의 PostGameplayEffectExecute에서 전담)
}

void UBaruHealthComponent::HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData)
{
    // ★[추가] 최대 체력 전용 델리게이트 통지
    OnMaxHealthChanged.Broadcast(ChangeData.OldValue, ChangeData.NewValue);

    // 최대 체력 변경 시에도 UI 갱신을 위해 현재 체력 기준으로 브로드캐스트
    const float CurrentHealth = GetHealth();
    OnHealthChanged.Broadcast(this, CurrentHealth, CurrentHealth, nullptr);
}