#include "AbilitySystem/BaruAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "Interfaces/CombatInterface.h"
#include "BaruLog.h"

UBaruAttributeSet::UBaruAttributeSet()
{
    InitHealth(100.0f);
    InitMaxHealth(100.0f);
    InitSanity(100.0f);
    InitMaxSanity(100.0f);
    InitSuppression(0.0f);
    InitMaxSuppression(100.0f);
    InitMoveSpeed(450.0f);
    InitPhysicalDefense(0.0f);
    InitCarryWeight(0.0f);
    InitMaxCarryWeight(40.0f);
    InitIncomingDamage(0.0f);
}

void UBaruAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UBaruAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBaruAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBaruAttributeSet, Sanity, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBaruAttributeSet, MaxSanity, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBaruAttributeSet, Suppression, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBaruAttributeSet, MaxSuppression, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBaruAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBaruAttributeSet, PhysicalDefense, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBaruAttributeSet, CarryWeight, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBaruAttributeSet, MaxCarryWeight, COND_None, REPNOTIFY_Always);
}

void UBaruAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

    // 어트리뷰트 변경 전 상/하한선 클램핑
    if (Attribute == GetHealthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
    }
    else if (Attribute == GetSanityAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxSanity());
    }
    else if (Attribute == GetSuppressionAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxSuppression());
    }
    else if (Attribute == GetCarryWeightAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f);
    }
    else if (Attribute == GetMaxHealthAttribute())
    {
        AdjustAttributeForMaxChange(Health, MaxHealth, NewValue, GetHealthAttribute());
    }
    else if (Attribute == GetMaxSanityAttribute())
    {
        AdjustAttributeForMaxChange(Sanity, MaxSanity, NewValue, GetSanityAttribute());
    }
}

void UBaruAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
    AActor* SourceActor = Context.GetOriginalInstigator();
    AActor* TargetActor = Data.Target.GetAvatarActor();

    // Meta Attributes
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
            
            if (NewHealth <= 0.0f && TargetActor && TargetActor->Implements<UCombatInterface>())
            {
                ICombatInterface::Execute_Die(TargetActor, SourceActor);
            }
        }
    }
    // Health
    else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    {
        SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
        if (GetHealth() <= 0.0f && TargetActor && TargetActor->Implements<UCombatInterface>())
        {
            ICombatInterface::Execute_Die(TargetActor, SourceActor);
        }
    }
    // Sanity
    else if (Data.EvaluatedData.Attribute == GetSanityAttribute())
    {
        SetSanity(FMath::Clamp(GetSanity(), 0.0f, GetMaxSanity()));
    }
    // Suppression
    else if (Data.EvaluatedData.Attribute == GetSuppressionAttribute())
    {
        SetSuppression(FMath::Clamp(GetSuppression(), 0.0f, GetMaxSuppression()));
    }
}

void UBaruAttributeSet::AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty)
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
void UBaruAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruAttributeSet, Health, OldHealth); }
void UBaruAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruAttributeSet, MaxHealth, OldMaxHealth); }
void UBaruAttributeSet::OnRep_Sanity(const FGameplayAttributeData& OldSanity) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruAttributeSet, Sanity, OldSanity); }
void UBaruAttributeSet::OnRep_MaxSanity(const FGameplayAttributeData& OldMaxSanity) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruAttributeSet, MaxSanity, OldMaxSanity); }
void UBaruAttributeSet::OnRep_Suppression(const FGameplayAttributeData& OldSuppression) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruAttributeSet, Suppression, OldSuppression); }
void UBaruAttributeSet::OnRep_MaxSuppression(const FGameplayAttributeData& OldMaxSuppression) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruAttributeSet, MaxSuppression, OldMaxSuppression); }
void UBaruAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruAttributeSet, MoveSpeed, OldMoveSpeed); }
void UBaruAttributeSet::OnRep_PhysicalDefense(const FGameplayAttributeData& OldPhysicalDefense) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruAttributeSet, PhysicalDefense, OldPhysicalDefense); }
void UBaruAttributeSet::OnRep_CarryWeight(const FGameplayAttributeData& OldCarryWeight) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruAttributeSet, CarryWeight, OldCarryWeight); }
void UBaruAttributeSet::OnRep_MaxCarryWeight(const FGameplayAttributeData& OldMaxCarryWeight) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruAttributeSet, MaxCarryWeight, OldMaxCarryWeight); }