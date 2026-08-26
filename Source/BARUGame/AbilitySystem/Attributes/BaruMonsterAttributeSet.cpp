#include "AbilitySystem/Attributes/BaruMonsterAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "GameplayTags/BaruGameplayTags.h"
#include "BaruLog.h"

UBaruMonsterAttributeSet::UBaruMonsterAttributeSet()
{
    InitSuppression(100.0f);
    InitMaxSuppression(100.0f);
    InitIncomingSuppressionDamage(0.0f);
}

void UBaruMonsterAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UBaruMonsterAttributeSet, Suppression, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UBaruMonsterAttributeSet, MaxSuppression, COND_None, REPNOTIFY_Always);
}

void UBaruMonsterAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

    if (Attribute == GetSuppressionAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxSuppression());
    }
    else if (Attribute == GetMaxSuppressionAttribute())
    {
        AdjustAttributeForMaxChange(Suppression, MaxSuppression, NewValue, GetSuppressionAttribute());
    }
}

void UBaruMonsterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    AActor* TargetActor = Data.Target.GetAvatarActor();
    UAbilitySystemComponent* TargetASC = Data.Target.AbilityActorInfo->AbilitySystemComponent.Get();

    if (Data.EvaluatedData.Attribute == GetIncomingSuppressionDamageAttribute())
    {
        const float LocalDamage = GetIncomingSuppressionDamage();
        SetIncomingSuppressionDamage(0.0f);

        if (LocalDamage > 0.0f)
        {
            const float NewSuppression = FMath::Clamp(GetSuppression() - LocalDamage, 0.0f, GetMaxSuppression());
            SetSuppression(NewSuppression);

            // 제압도 0 도달 시 그로기 태그 부여
            if (NewSuppression <= 0.0f && TargetASC)
            {
                const FGameplayTag GroggyTag = FGameplayTag::RequestGameplayTag(TEXT("State.Debuff.Groggy"));
                if (!TargetASC->HasMatchingGameplayTag(GroggyTag))
                {
                    TargetASC->AddLooseGameplayTag(GroggyTag);
                    BARU_NET_LOG(TargetActor, LogBaruCombat, Log, TEXT("Monster Entered Groggy State!"));
                }
            }
        }
    }
}

void UBaruMonsterAttributeSet::AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty)
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

void UBaruMonsterAttributeSet::OnRep_Suppression(const FGameplayAttributeData& OldSuppression) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruMonsterAttributeSet, Suppression, OldSuppression); }
void UBaruMonsterAttributeSet::OnRep_MaxSuppression(const FGameplayAttributeData& OldMaxSuppression) { GAMEPLAYATTRIBUTE_REPNOTIFY(UBaruMonsterAttributeSet, MaxSuppression, OldMaxSuppression); }