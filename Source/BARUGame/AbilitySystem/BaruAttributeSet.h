#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "BaruAttributeSet.generated.h"

// GAS Attribute 매크로 정의
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 인게임 핵심 속성 관리
 */
UCLASS()
class BARUGAME_API UBaruAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UBaruAttributeSet();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    // ==============================================================================
    // Gameplay Attributes (Health / Sanity / Suppression)
    // ==============================================================================
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "BARU|Attributes|Health")
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UBaruAttributeSet, Health)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "BARU|Attributes|Health")
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(UBaruAttributeSet, MaxHealth)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Sanity, Category = "BARU|Attributes|Sanity")
    FGameplayAttributeData Sanity;
    ATTRIBUTE_ACCESSORS(UBaruAttributeSet, Sanity)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxSanity, Category = "BARU|Attributes|Sanity")
    FGameplayAttributeData MaxSanity;
    ATTRIBUTE_ACCESSORS(UBaruAttributeSet, MaxSanity)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Suppression, Category = "BARU|Attributes|Combat")
    FGameplayAttributeData Suppression;
    ATTRIBUTE_ACCESSORS(UBaruAttributeSet, Suppression)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxSuppression, Category = "BARU|Attributes|Combat")
    FGameplayAttributeData MaxSuppression;
    ATTRIBUTE_ACCESSORS(UBaruAttributeSet, MaxSuppression)

    // ==============================================================================
    // Status Attributes (MoveSpeed / PhysicalDefense / CarryWeight)
    // ==============================================================================
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeed, Category = "BARU|Attributes|Movement")
    FGameplayAttributeData MoveSpeed;
    ATTRIBUTE_ACCESSORS(UBaruAttributeSet, MoveSpeed)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PhysicalDefense, Category = "BARU|Attributes|Combat")
    FGameplayAttributeData PhysicalDefense;
    ATTRIBUTE_ACCESSORS(UBaruAttributeSet, PhysicalDefense)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CarryWeight, Category = "BARU|Attributes|Inventory")
    FGameplayAttributeData CarryWeight;
    ATTRIBUTE_ACCESSORS(UBaruAttributeSet, CarryWeight)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxCarryWeight, Category = "BARU|Attributes|Inventory")
    FGameplayAttributeData MaxCarryWeight;
    ATTRIBUTE_ACCESSORS(UBaruAttributeSet, MaxCarryWeight)

    // ==============================================================================
    // Meta Attributes ( [Server Only] Damage Calculation )
    // ==============================================================================
    UPROPERTY(BlueprintReadOnly, Category = "BARU|Attributes|Meta")
    FGameplayAttributeData IncomingDamage;
    ATTRIBUTE_ACCESSORS(UBaruAttributeSet, IncomingDamage)

protected:
    UFUNCTION() virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);
    UFUNCTION() virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);
    UFUNCTION() virtual void OnRep_Sanity(const FGameplayAttributeData& OldSanity);
    UFUNCTION() virtual void OnRep_MaxSanity(const FGameplayAttributeData& OldMaxSanity);
    UFUNCTION() virtual void OnRep_Suppression(const FGameplayAttributeData& OldSuppression);
    UFUNCTION() virtual void OnRep_MaxSuppression(const FGameplayAttributeData& OldMaxSuppression);
    UFUNCTION() virtual void OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed);
    UFUNCTION() virtual void OnRep_PhysicalDefense(const FGameplayAttributeData& OldPhysicalDefense);
    UFUNCTION() virtual void OnRep_CarryWeight(const FGameplayAttributeData& OldCarryWeight);
    UFUNCTION() virtual void OnRep_MaxCarryWeight(const FGameplayAttributeData& OldMaxCarryWeight);

private:
    // Helper
    void AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty);
};