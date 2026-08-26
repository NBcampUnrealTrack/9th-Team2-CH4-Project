#include "AbilitySystem/Executions/BaruDamageExecutionCalc.h"
#include "AbilitySystem/Attributes/BaruCoreAttributeSet.h"
#include "AbilitySystem/Attributes/BaruMonsterAttributeSet.h"
#include "AbilitySystem/BaruAbilitySystemComponent.h"
#include "GameplayTags/BaruGameplayTags.h"
#include "BaruLog.h"

struct FBaruDamageStatics
{
    DECLARE_ATTRIBUTE_CAPTUREDEF(PhysicalDefense);
    DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);
    
    // 몬스터 전용 SuppressionDamage Capture
    DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingSuppressionDamage);

    FBaruDamageStatics()
    {
        // PhysicalDefense
        DEFINE_ATTRIBUTE_CAPTUREDEF(UBaruCoreAttributeSet, PhysicalDefense, Target, false);

        // IncomingDamage
        DEFINE_ATTRIBUTE_CAPTUREDEF(UBaruCoreAttributeSet, IncomingDamage, Target, false);
        
        // [Monster] Suppression Damage
        DEFINE_ATTRIBUTE_CAPTUREDEF(UBaruMonsterAttributeSet, IncomingSuppressionDamage, Target, false);
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
    RelevantAttributesToCapture.Add(DamageStatics().IncomingSuppressionDamageDef);
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

    // ==============================================================================
    // 물리 데미지 계산 (Physical Defense 기반 감쇄)
    // ==============================================================================
    
    // PhysicalDefense
    float TargetDefense = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().PhysicalDefenseDef, EvaluationParameters, TargetDefense);
    TargetDefense = FMath::Max(TargetDefense, 0.0f);

    // BaseDamage
    const float BasePhysicalDamage = FMath::Max(Spec.GetSetByCallerMagnitude(FBaruGameplayTags::Get().Data_Damage, false, 0.0f), 0.0f);
    
    // Calculation
    if (BasePhysicalDamage > 0.0f)
    {
        const float DefenseMitigation = TargetDefense / (TargetDefense + 100.0f);
        const float FinalPhysicalDamage = FMath::Max(BasePhysicalDamage * (1.0f - DefenseMitigation), 0.0f);

        if (FinalPhysicalDamage > 0.0f)
        {
            OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(DamageStatics().IncomingDamageProperty, EGameplayModOp::Additive, FinalPhysicalDamage));
        }
    }
    
    // ==============================================================================
    // 2. 제압(Suppression / 그로기) 데미지 계산
    // ==============================================================================
    
    // Todo : 명확한 제압 그로기 로직 합의 필요
    // 현재 기본값은 별도의 제압치가 없는 경우 물리 데미지의 50% 부여 + 제압치가 있는 경우 100% 부여
    const FGameplayTag SuppressionDamageTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage.Suppression"));
    float BaseSuppressionDamage = Spec.GetSetByCallerMagnitude(SuppressionDamageTag, false, -1.0f);

    if (BaseSuppressionDamage < 0.0f)
    {
        // 제압 수치가 명시되지 않은 일반 사격/타격 시 물리 데미지의 절반을 제압치로 환산
        BaseSuppressionDamage = BasePhysicalDamage * 0.5f;
    }

    if (BaseSuppressionDamage > 0.0f)
    {
        OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(DamageStatics().IncomingSuppressionDamageProperty, EGameplayModOp::Additive, BaseSuppressionDamage));
    }
}