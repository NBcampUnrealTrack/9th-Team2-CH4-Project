#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "BaruMonsterAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 몬스터 전용 특수 스탯
 */
UCLASS()
class BARUGAME_API UBaruMonsterAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UBaruMonsterAttributeSet();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    // 제압도 게이지
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Suppression, Category = "BARU|Monster|Suppression")
    FGameplayAttributeData Suppression;
    ATTRIBUTE_ACCESSORS(UBaruMonsterAttributeSet, Suppression)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxSuppression, Category = "BARU|Monster|Suppression")
    FGameplayAttributeData MaxSuppression;
    ATTRIBUTE_ACCESSORS(UBaruMonsterAttributeSet, MaxSuppression)

    // [Server Only] 제압 데미지 메타 속성
    UPROPERTY(BlueprintReadOnly, Category = "BARU|Monster|Meta")
    FGameplayAttributeData IncomingSuppressionDamage;
    ATTRIBUTE_ACCESSORS(UBaruMonsterAttributeSet, IncomingSuppressionDamage)

protected:
    UFUNCTION() virtual void OnRep_Suppression(const FGameplayAttributeData& OldSuppression);
    UFUNCTION() virtual void OnRep_MaxSuppression(const FGameplayAttributeData& OldMaxSuppression);

private:
    void AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty);
};