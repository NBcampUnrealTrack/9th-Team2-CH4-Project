#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "BaruCoreAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 공통으로 소유하는 Core 스탯 및 속성 값 정의
 */
UCLASS()
class BARUGAME_API UBaruCoreAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UBaruCoreAttributeSet();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

    // 체력
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "BARU|Core|Health")
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UBaruCoreAttributeSet, Health)

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "BARU|Core|Health")
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(UBaruCoreAttributeSet, MaxHealth)

    // 물리 방어력
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PhysicalDefense, Category = "BARU|Core|Combat")
    FGameplayAttributeData PhysicalDefense;
    ATTRIBUTE_ACCESSORS(UBaruCoreAttributeSet, PhysicalDefense)

    // Todo : 특수 방어력 정의 후 속성 값 알맞은 곳에 추가
    
    // 이동 속도
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeed, Category = "BARU|Core|Movement")
    FGameplayAttributeData MoveSpeed;
    ATTRIBUTE_ACCESSORS(UBaruCoreAttributeSet, MoveSpeed)

    // [Server Only] 데미지 계산 메타 속성
    UPROPERTY(BlueprintReadOnly, Category = "BARU|Core|Meta")
    FGameplayAttributeData IncomingDamage;
    ATTRIBUTE_ACCESSORS(UBaruCoreAttributeSet, IncomingDamage)

protected:
    UFUNCTION() virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);
    UFUNCTION() virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);
    UFUNCTION() virtual void OnRep_PhysicalDefense(const FGameplayAttributeData& OldPhysicalDefense);
    UFUNCTION() virtual void OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed);

private:
    // Helper
    void AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty);
};