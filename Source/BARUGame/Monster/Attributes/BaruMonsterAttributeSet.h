

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "BaruMonsterAttributeSet.generated.h"


// 몬스터 Attribute에 필요한 Getter와 Setter를 자동 생성
#define BARU_MONSTER_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)


//몬스터의 체력, 제압, 방어도 같은 전투 수치를 보관하는 AttributeSet
UCLASS()
class BARUGAME_API UBaruMonsterAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UBaruMonsterAttributeSet();
	
	// 복제할 Attribute 목록을 서버에 등록
    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps
    ) const override;

    // 현재 체력
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "BARU|Monster|Attributes|Health")
    FGameplayAttributeData Health;

    BARU_MONSTER_ATTRIBUTE_ACCESSORS(UBaruMonsterAttributeSet, Health)

    // 최대 체력
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "BARU|Monster|Attributes|Health")
    FGameplayAttributeData MaxHealth;

    BARU_MONSTER_ATTRIBUTE_ACCESSORS(UBaruMonsterAttributeSet, MaxHealth)

    // 현재 제압 게이지
    // 0이 되면 제압 상태에 진입
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Suppression, Category = "BARU|Monster|Attributes|Suppression")
    FGameplayAttributeData Suppression;

    BARU_MONSTER_ATTRIBUTE_ACCESSORS(UBaruMonsterAttributeSet, Suppression)

    // 최대 제압 게이지
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxSuppression, Category = "BARU|Monster|Attributes|Suppression")
    FGameplayAttributeData MaxSuppression;

    BARU_MONSTER_ATTRIBUTE_ACCESSORS(UBaruMonsterAttributeSet, MaxSuppression)

    // 물리 피해를 감소시키는 방어도
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PhysicalDefense, Category = "BARU|Monster|Attributes|Defense")
    FGameplayAttributeData PhysicalDefense;

    BARU_MONSTER_ATTRIBUTE_ACCESSORS(UBaruMonsterAttributeSet, PhysicalDefense)

    // 받은 체력 피해를 임시로 전달하는 서버 전용 값
    // 실제 체력에서 차감한 뒤 다시 0으로 초기화
    UPROPERTY(BlueprintReadOnly, Category = "BARU|Monster|Attributes|Meta")
    FGameplayAttributeData IncomingDamage;

    BARU_MONSTER_ATTRIBUTE_ACCESSORS(UBaruMonsterAttributeSet,IncomingDamage)

    // 받은 제압 피해를 임시로 전달하는 서버 전용 값
    UPROPERTY(BlueprintReadOnly, Category = "BARU|Monster|Attributes|Meta")
    FGameplayAttributeData IncomingSuppressionDamage;

    BARU_MONSTER_ATTRIBUTE_ACCESSORS(UBaruMonsterAttributeSet, IncomingSuppressionDamage)

protected:
    // 서버에서 복제된 값이 클라이언트에서 변경될 때 호출
    UFUNCTION()
    void OnRep_Health(const FGameplayAttributeData& OldHealth);

    UFUNCTION()
    void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

    UFUNCTION()
    void OnRep_Suppression(const FGameplayAttributeData& OldSuppression);

    UFUNCTION()
    void OnRep_MaxSuppression(const FGameplayAttributeData& OldMaxSuppression);

    UFUNCTION()
    void OnRep_PhysicalDefense(const FGameplayAttributeData& OldPhysicalDefense);
};

#undef BARU_MONSTER_ATTRIBUTE_ACCESSORS
