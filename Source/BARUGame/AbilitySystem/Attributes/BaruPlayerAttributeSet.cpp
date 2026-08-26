#include "AbilitySystem/Attributes/BaruPlayerAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

UBaruPlayerAttributeSet::UBaruPlayerAttributeSet()
{
    InitSanity(100.0f);
    InitMaxSanity(100.0f);
    InitCarryWeight(0.0f);
    InitMaxCarryWeight(40.0f);
    InitTension(0.0f);
    InitMaxTension(100.0f);
}

void UBaruPlayerAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UBaruPlayerAttributeSet, Sanity, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBaruPlayerAttributeSet, MaxSanity, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBaruPlayerAttributeSet, Tension, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBaruPlayerAttributeSet, MaxTension, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBaruPlayerAttributeSet, CarryWeight, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBaruPlayerAttributeSet, MaxCarryWeight, COND_None, REPNOTIFY_Always);
}

void UBaruPlayerAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

    if (Attribute == GetSanityAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxSanity());
    }
    else if (Attribute == GetTensionAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxTension());
    }
    else if (Attribute == GetCarryWeightAttribute())
    {
        NewValue = FMath::Max(NewValue, 0.0f);
    }
    else if (Attribute == GetMaxSanityAttribute())
    {
        AdjustAttributeForMaxChange(Sanity, MaxSanity, NewValue, GetSanityAttribute());
    }
    else if (Attribute == GetMaxTensionAttribute())
    {
        AdjustAttributeForMaxChange(Tension, MaxTension, NewValue, GetTensionAttribute());
    }
}

void UBaruPlayerAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    if (Data.EvaluatedData.Attribute == GetSanityAttribute())
    {
        SetSanity(FMath::Clamp(GetSanity(), 0.0f, GetMaxSanity()));
    }
    else if (Data.EvaluatedData.Attribute == GetCarryWeightAttribute())
    {
        SetCarryWeight(FMath::Max(GetCarryWeight(), 0.0f));
    }
    else if (Data.EvaluatedData.Attribute == GetTensionAttribute())
    {
        SetTension(FMath::Clamp(GetTension(), 0.0f, GetMaxTension()));
    }
}

void UBaruPlayerAttributeSet::AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty)
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

void UBaruPlayerAttributeSet::OnRep_Sanity(const FGameplayAttributeData& OldSanity) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruPlayerAttributeSet, Sanity, OldSanity); }
void UBaruPlayerAttributeSet::OnRep_MaxSanity(const FGameplayAttributeData& OldMaxSanity) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruPlayerAttributeSet, MaxSanity, OldMaxSanity); }
void UBaruPlayerAttributeSet::OnRep_Tension(const FGameplayAttributeData& OldTension) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruPlayerAttributeSet, Tension, OldTension); }
void UBaruPlayerAttributeSet::OnRep_MaxTension(const FGameplayAttributeData& OldMaxTension) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruPlayerAttributeSet, MaxTension, OldMaxTension); }
void UBaruPlayerAttributeSet::OnRep_CarryWeight(const FGameplayAttributeData& OldCarryWeight) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruPlayerAttributeSet, CarryWeight, OldCarryWeight); }
void UBaruPlayerAttributeSet::OnRep_MaxCarryWeight(const FGameplayAttributeData& OldMaxCarryWeight) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruPlayerAttributeSet, MaxCarryWeight, OldMaxCarryWeight); }