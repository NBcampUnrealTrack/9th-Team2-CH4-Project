#include "AbilitySystem/Executions/BaruDamageExecutionCalc.h"
#include "AbilitySystem/Attributes/BaruCoreAttributeSet.h"
#include "AbilitySystem/Attributes/BaruMonsterAttributeSet.h"
#include "AbilitySystem/Attributes/BaruPlayerAttributeSet.h"
#include "AbilitySystem/BaruAbilitySystemComponent.h"
#include "GameplayTags/BaruGameplayTags.h"
#include "BaruLog.h"

struct FBaruDamageStatics
{
    DECLARE_ATTRIBUTE_CAPTUREDEF(PhysicalDefense);
    DECLARE_ATTRIBUTE_CAPTUREDEF(SpecialResistance);
    DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);
    
    // 몬스터 전용 SuppressionDamage Capture
    DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingSuppressionDamage);

    FBaruDamageStatics()
    {
        // PhysicalDefense
        DEFINE_ATTRIBUTE_CAPTUREDEF(UBaruCoreAttributeSet, PhysicalDefense, Target, false);

        // SpecialResistance
        DEFINE_ATTRIBUTE_CAPTUREDEF(UBaruCoreAttributeSet, SpecialResistance, Target, false);
        
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
    RelevantAttributesToCapture.Add(DamageStatics().SpecialResistanceDef);
    RelevantAttributesToCapture.Add(DamageStatics().IncomingDamageDef);
    RelevantAttributesToCapture.Add(DamageStatics().IncomingSuppressionDamageDef);
}

void UBaruDamageExecutionCalc::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
    
    // ==============================================================================
    // 팀킬 / 아군 피견 검사 로직
    // ==============================================================================
    UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
    UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
    
    // 피격 대상의 State.Immune 상태 검사
    if (TargetASC && TargetASC->HasMatchingGameplayTag(FBaruGameplayTags::Get().State_Immune))
    {
        BARU_NET_LOG(TargetASC->GetAvatarActor(), LogBaruCombat, Verbose, 
            TEXT("Target is IMMUNE. Damage ignored."));
        return; // 데미지 계산 중단 (피해 0)
    }
    
    if (TargetASC && SourceASC && TargetASC != SourceASC)
    {
        const UBaruPlayerAttributeSet* TargetPlayerSet = TargetASC->GetSet<UBaruPlayerAttributeSet>();
        const UBaruPlayerAttributeSet* SourcePlayerSet = SourceASC->GetSet<UBaruPlayerAttributeSet>();

        // 플레이어 간 상호작용인 경우
        if (TargetPlayerSet != nullptr && SourcePlayerSet != nullptr)
        {
            const float AttackerSanity = SourcePlayerSet->GetSanity();
            const FGameplayTag FrenzyTag = FBaruGameplayTags::Get().State_Sanity_Frenzy;

            // 광란 상태 조건
            const bool bIsAttackerInFrenzy = (AttackerSanity <= 20.0f) || SourceASC->HasMatchingGameplayTag(FrenzyTag);

            // 광란 상태가 아닐 경우
            if (!bIsAttackerInFrenzy)
            {
                BARU_NET_LOG(TargetASC->GetAvatarActor(), LogBaruCombat, Log,
                    TEXT("[FRIENDLY_FIRE_BLOCKED] Attacker '%s' Sanity: %.1f > 20.0. Team damage prevented."),
                    *GetNameSafe(SourceASC->GetAvatarActor()), AttackerSanity);
                return;
            }

            BARU_NET_LOG(TargetASC->GetAvatarActor(), LogBaruCombat, Warning,
                TEXT("[FRIENDLY_FIRE_ALLOWED] Attacker '%s' is in FRENZY. (Sanity: %.1f <= 20.0). Team damage applied."),
                *GetNameSafe(SourceASC->GetAvatarActor()), AttackerSanity);
        }
    }
    
    
    
    
    FAggregatorEvaluateParameters EvaluationParameters;
    
    // Source/Target 태그 컨테이너 조회
    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    float TotalDamageToApply = 0.0f;
    
    // ==============================================================================
    // 물리 데미지 계산 (감소율 = 0.95 * log10(1 + 방어) / log10(100001))
    // ==============================================================================
    
    // PhysicalDefense
    float TargetDefense = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().PhysicalDefenseDef, EvaluationParameters, TargetDefense);
    TargetDefense = FMath::Max(TargetDefense, 0.0f);

    // BaseDamage
    const float BasePhysicalDamage = FMath::Max(Spec.GetSetByCallerMagnitude(FBaruGameplayTags::Get().Data_Damage, false, 0.0f), 0.0f);
    
    // Calculation
    float FinalPhysicalDamage = 0.0f;
    if (BasePhysicalDamage > 0.0f)
    {
        // log10(100001) 상수 (약 5.00000434)
        constexpr float Log10_100001 = 5.00000434f;
        
        // 감소율 계산 (0.0 ~ 0.95 클램핑)
        const float DefenseMitigation = FMath::Clamp(
            0.95f * (FMath::LogX(10.0f, 1.0f + TargetDefense) / Log10_100001),
            0.0f,
            0.95f
        );

        FinalPhysicalDamage = FMath::Max(BasePhysicalDamage * (1.0f - DefenseMitigation), 0.0f);
        TotalDamageToApply += FinalPhysicalDamage;
    }
    
    // ==============================================================================
    // 특수형 방어 계산: 특수 최종 피해 = 기본 특수 피해 * (1 - 저항률)
    // ==============================================================================
    float TargetResistance = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().SpecialResistanceDef, EvaluationParameters, TargetResistance);
    TargetResistance = FMath::Clamp(TargetResistance, 0.0f, 1.0f);

    // 특수 피해 태그 조회
    const FGameplayTag SpecialDamageTag = FBaruGameplayTags::Get().Data_Damage_Special;
    const float BaseSpecialDamage = SpecialDamageTag.IsValid() 
        ? FMath::Max(Spec.GetSetByCallerMagnitude(SpecialDamageTag, false, 0.0f), 0.0f) 
        : 0.0f;

    if (BaseSpecialDamage > 0.0f)
    {
        const float FinalSpecialDamage = FMath::Max(BaseSpecialDamage * (1.0f - TargetResistance), 0.0f);
        TotalDamageToApply += FinalSpecialDamage;
    }
    
    // ==============================================================================
    // 최종 체력 데미지 전달 (물리 + 특수 합산)
    // ==============================================================================
    if (TotalDamageToApply > 0.0f)
    {
        OutExecutionOutput.AddOutputModifier(
            FGameplayModifierEvaluatedData(DamageStatics().IncomingDamageProperty, EGameplayModOp::Additive, TotalDamageToApply)
        );
    }
    
    // ==============================================================================
    // 제압도(Suppression) 연계
    // ==============================================================================
    const FGameplayTag SuppressionDamageTag = FBaruGameplayTags::Get().Data_Damage_Suppression;
    float BaseSuppressionDamage = SuppressionDamageTag.IsValid() 
        ? Spec.GetSetByCallerMagnitude(SuppressionDamageTag, false, -1.0f) 
        : -1.0f;

    // 명시적 제압 피해가 없으면 실질 물리 피해의 50%를 적용
    if (BaseSuppressionDamage < 0.0f)
    {
        BaseSuppressionDamage = FinalPhysicalDamage * 0.5f;
    }

    if (BaseSuppressionDamage > 0.0f && TargetASC && TargetASC->GetSet<UBaruMonsterAttributeSet>())
    {
        OutExecutionOutput.AddOutputModifier(
            FGameplayModifierEvaluatedData(DamageStatics().IncomingSuppressionDamageProperty, EGameplayModOp::Additive, BaseSuppressionDamage)
        );
    }
}