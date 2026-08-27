#include "BaruHealthComponent.h"
#include "AbilitySystemComponent.h"
#include "Player/BaruPlayerState.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystem/Attributes/BaruCoreAttributeSet.h"

UBaruHealthComponent::UBaruHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    bIsDead = false;
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

void UBaruHealthComponent::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
    AActor* Instigator = nullptr;

    if (ChangeData.GEModData)
    {
        const FGameplayEffectContextHandle& EffectContext = ChangeData.GEModData->EffectSpec.GetEffectContext();
        Instigator = EffectContext.GetInstigator();
    }

    OnHealthChanged.Broadcast(this, ChangeData.OldValue, ChangeData.NewValue, Instigator);

    // 데미지를 입고 처음 체력이 0 이하가 되었을 때 한 번만 사망 판정
    if (ChangeData.NewValue <= 0.0f && !bIsDead)
    {
        bIsDead = true;
        OnDeath.Broadcast(Instigator);
    }
    // 부활/회복 시 사망 상태 해제
    else if (ChangeData.NewValue > 0.0f && bIsDead)
    {
        bIsDead = false;
    }
}

void UBaruHealthComponent::HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData)
{
    // 최대 체력 변경 시에도 UI 갱신을 위해 현재 체력 기준으로 브로드캐스트
    const float CurrentHealth = GetHealth();
    OnHealthChanged.Broadcast(this, CurrentHealth, CurrentHealth, nullptr);
}