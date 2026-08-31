#include "AbilitySystem/Attributes/BaruCoreAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Interfaces/CombatInterface.h"
#include "BaruLog.h"

UBaruCoreAttributeSet::UBaruCoreAttributeSet()
{
    InitHealth(100.0f);
    InitMaxHealth(100.0f);
    InitPhysicalDefense(0.0f);
    InitSpecialResistance(0.0f);
    InitMoveSpeed(450.0f);
    InitIncomingDamage(0.0f);
}

void UBaruCoreAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UBaruCoreAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBaruCoreAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBaruCoreAttributeSet, PhysicalDefense, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBaruCoreAttributeSet, SpecialResistance, COND_None, REPNOTIFY_Always); // [추가]
    DOREPLIFETIME_CONDITION_NOTIFY(UBaruCoreAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
}

void UBaruCoreAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

    if (Attribute == GetHealthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
    }
    else if (Attribute == GetPhysicalDefenseAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f);
    }
    else if (Attribute == GetSpecialResistanceAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
    }
    else if (Attribute == GetMaxHealthAttribute())
    {
        AdjustAttributeForMaxChange(Health, MaxHealth, NewValue, GetHealthAttribute());
    }
}

void UBaruCoreAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
    AActor* SourceActor = Context.GetOriginalInstigator();
    AActor* TargetActor = Data.Target.GetAvatarActor();

    // 메타 데미지 반영
    if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
    {
        const float LocalIncomingDamage = GetIncomingDamage();
        SetIncomingDamage(0.0f);

        if (LocalIncomingDamage > 0.0f)
        {
            const float OldHealth = GetHealth();
            const float NewHealth = FMath::Clamp(OldHealth - LocalIncomingDamage, 0.0f, GetMaxHealth());
            SetHealth(NewHealth);

            BARU_NET_LOG(TargetActor, LogBaruCombat, Log, TEXT("Damage Applied: %.1f | Health: %.1f -> %.1f"), LocalIncomingDamage, OldHealth, NewHealth);

            // 사망 검증 및 1회만 Die 인터페이스 호출
            if (NewHealth <= 0.0f && TargetActor && TargetActor->Implements<UCombatInterface>())
            {
                if (!ICombatInterface::Execute_IsDead(TargetActor))
                {
                    ICombatInterface::Execute_Die(TargetActor, SourceActor);
                }
            }
        }
    }
    else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    {
        SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
        if (GetHealth() <= 0.0f && TargetActor && TargetActor->Implements<UCombatInterface>())
        {
            if (!ICombatInterface::Execute_IsDead(TargetActor))
            {
                ICombatInterface::Execute_Die(TargetActor, SourceActor);
            }
        }
    }
}

void UBaruCoreAttributeSet::AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty)
{
    UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
    const float CurrentMaxValue = MaxAttribute.GetCurrentValue();
    if (!FMath::IsNearlyEqual(CurrentMaxValue, NewMaxValue) && ASC)
    {
        const float CurrentValue = AffectedAttribute.GetCurrentValue();
        const float NewDelta = (CurrentMaxValue > 0.0f) ? (CurrentValue * (NewMaxValue / CurrentMaxValue)) - CurrentValue : NewMaxValue;
        ASC->ApplyModToAttributeUnsafe(AffectedAttributeProperty, EGameplayModOp::Additive, NewDelta);
    }
}

// ==============================================================================
// RepNotify 구현
// ==============================================================================
void UBaruCoreAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruCoreAttributeSet, Health, OldHealth); }
void UBaruCoreAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruCoreAttributeSet, MaxHealth, OldMaxHealth); }
void UBaruCoreAttributeSet::OnRep_PhysicalDefense(const FGameplayAttributeData& OldPhysicalDefense) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruCoreAttributeSet, PhysicalDefense, OldPhysicalDefense); }
void UBaruCoreAttributeSet::OnRep_SpecialResistance(const FGameplayAttributeData& OldSpecialResistance) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruCoreAttributeSet, SpecialResistance, OldSpecialResistance); }
void UBaruCoreAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruCoreAttributeSet, MoveSpeed, OldMoveSpeed); }