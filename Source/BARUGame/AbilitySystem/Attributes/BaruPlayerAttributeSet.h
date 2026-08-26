#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "BaruPlayerAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 플레이어 전용 생존/파밍 스탯
 */
UCLASS()
class BARUGAME_API UBaruPlayerAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UBaruPlayerAttributeSet();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    // 정신력 (Sanity)
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Sanity, Category = "BARU|Player|Sanity")
    FGameplayAttributeData Sanity;
    ATTRIBUTE_ACCESSORS(UBaruPlayerAttributeSet, Sanity)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxSanity, Category = "BARU|Player|Sanity")
    FGameplayAttributeData MaxSanity;
    ATTRIBUTE_ACCESSORS(UBaruPlayerAttributeSet, MaxSanity)

    // 소지 무게 (CarryWeight)
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CarryWeight, Category = "BARU|Player|Inventory")
    FGameplayAttributeData CarryWeight;
    ATTRIBUTE_ACCESSORS(UBaruPlayerAttributeSet, CarryWeight)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxCarryWeight, Category = "BARU|Player|Inventory")
    FGameplayAttributeData MaxCarryWeight;
    ATTRIBUTE_ACCESSORS(UBaruPlayerAttributeSet, MaxCarryWeight)
    
    // 긴장도 (Tension) - 기본 0 
    // Todo: 몬스터 근접, 피격 시 상승 / 심박수 증가 및 조준 흔들림 유발
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Tension, Category = "BARU|Player|Tension")
    FGameplayAttributeData Tension;
    ATTRIBUTE_ACCESSORS(UBaruPlayerAttributeSet, Tension)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxTension, Category = "BARU|Player|Tension")
    FGameplayAttributeData MaxTension;
    ATTRIBUTE_ACCESSORS(UBaruPlayerAttributeSet, MaxTension)

protected:
    UFUNCTION() virtual void OnRep_Sanity(const FGameplayAttributeData& OldSanity);
    UFUNCTION() virtual void OnRep_MaxSanity(const FGameplayAttributeData& OldMaxSanity);
    UFUNCTION() virtual void OnRep_CarryWeight(const FGameplayAttributeData& OldCarryWeight);
    UFUNCTION() virtual void OnRep_MaxCarryWeight(const FGameplayAttributeData& OldMaxCarryWeight);
    UFUNCTION() virtual void OnRep_Tension(const FGameplayAttributeData& OldTension);
    UFUNCTION() virtual void OnRep_MaxTension(const FGameplayAttributeData& OldMaxTension);

private:
    void AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty);
};