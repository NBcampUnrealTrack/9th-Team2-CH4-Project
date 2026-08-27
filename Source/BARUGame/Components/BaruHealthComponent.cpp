#include "BaruHealthComponent.h"
#include "AbilitySystemComponent.h"
#include "Player/BaruPlayerState.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystem/Attributes/BaruCoreAttributeSet.h"
#include "Interfaces/CombatInterface.h" 


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

// [수정] IsDead() 함수 구현: CombatInterface를 통해 사망 여부를 정확히 쿼리합니다.
bool UBaruHealthComponent::IsDead() const
{
    if (AActor* Owner = GetOwner())
    {
        if (Owner->Implements<UCombatInterface>())
        {
            return ICombatInterface::Execute_IsDead(Owner);
        }
    }
    return false;
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

    // [삭제] 사망 판정 로직 삭제 (BaruCoreAttributeSet의 PostGameplayEffectExecute에서 전담하여 Die 인터페이스 호출함)
}

void UBaruHealthComponent::HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData)
{
    // 최대 체력 변경 시에도 UI 갱신을 위해 현재 체력 기준으로 브로드캐스트
    const float CurrentHealth = GetHealth();
    OnHealthChanged.Broadcast(this, CurrentHealth, CurrentHealth, nullptr);
}

