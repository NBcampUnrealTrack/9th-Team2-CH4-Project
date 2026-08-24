#include "AbilitySystem/Executions/BaruDamageExecutionCalc.h"
#include "AbilitySystem/BaruAttributeSet.h"
#include "AbilitySystem/BaruAbilitySystemComponent.h"
#include "BaruLog.h"

struct FBaruDamageStatics
{
    DECLARE_ATTRIBUTE_CAPTUREDEF(PhysicalDefense);
    DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);

    FBaruDamageStatics()
    {
        // PhysicalDefense
        DEFINE_ATTRIBUTE_CAPTUREDEF(UBaruAttributeSet, PhysicalDefense, Target, false);

        // IncomingDamage
        DEFINE_ATTRIBUTE_CAPTUREDEF(UBaruAttributeSet, IncomingDamage, Target, false);
    }
};

static const FBaruDamageStatics& DamageStatics()
{
    static FBaruDamageStatics DStatics;
    return DStatics;
}

UBaruDamageExecutionCalc::UBaruDamageExecutionCalc()
{
    RelevantAttributesToCapture.Add(DamageStatics().PhysicalDefenseDef);
    RelevantAttributesToCapture.Add(DamageStatics().IncomingDamageDef);
}

void UBaruDamageExecutionCalc::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
    
    // Source/Target 태그 컨테이너 조회
    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
    
    FAggregatorEvaluateParameters EvaluationParameters;
    EvaluationParameters.SourceTags = SourceTags;
    EvaluationParameters.TargetTags = TargetTags;

    // PhysicalDefense
    float TargetDefense = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().PhysicalDefenseDef, EvaluationParameters, TargetDefense);
    TargetDefense = FMath::Max(TargetDefense, 0.0f);

    // BaseDamage
    float BaseDamage = FMath::Max(Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("Data.Damage")), false, 0.0f), 0.0f);

    // Calculation
    const float DefenseMitigation = TargetDefense / (TargetDefense + 100.0f);
    const float FinalDamage = FMath::Max(BaseDamage * (1.0f - DefenseMitigation), 0.0f);

    if (FinalDamage > 0.0f)
    {
        // Target의 IncomingDamage 메타 속성에 가산
        OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(DamageStatics().IncomingDamageProperty, EGameplayModOp::Additive, FinalDamage));
    }
}