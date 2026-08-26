


#include "Monster/Attributes/BaruMonsterAttributeSet.h"
#include "Net/UnrealNetwork.h"


UBaruMonsterAttributeSet::UBaruMonsterAttributeSet()
{
	// 아직 초기 스탯 GameplayEffect가 없으므로
	// 테스트에 사용할 안전한 기본값을 지정
	InitMaxHealth(100.0f);
	InitHealth(100.0f);

	// 제압 게이지는 최대치에서 시작
	InitMaxSuppression(100.0f);
	InitSuppression(100.0f);

	InitPhysicalDefense(0.0f);

	// Meta Attribute는 항상 0에서 시작
	InitIncomingDamage(0.0f);
	InitIncomingSuppressionDamage(0.0f);
}

void UBaruMonsterAttributeSet::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps
) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(
        UBaruMonsterAttributeSet,
        Health,
        COND_None,
        REPNOTIFY_Always
    );

    DOREPLIFETIME_CONDITION_NOTIFY(
        UBaruMonsterAttributeSet,
        MaxHealth,
        COND_None,
        REPNOTIFY_Always
    );

    DOREPLIFETIME_CONDITION_NOTIFY(
        UBaruMonsterAttributeSet,
        Suppression,
        COND_None,
        REPNOTIFY_Always
    );

    DOREPLIFETIME_CONDITION_NOTIFY(
        UBaruMonsterAttributeSet,
        MaxSuppression,
        COND_None,
        REPNOTIFY_Always
    );

    DOREPLIFETIME_CONDITION_NOTIFY(
        UBaruMonsterAttributeSet,
        PhysicalDefense,
        COND_None,
        REPNOTIFY_Always
    );
}

void UBaruMonsterAttributeSet::OnRep_Health(
    const FGameplayAttributeData& OldHealth
)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(
        UBaruMonsterAttributeSet,
        Health,
        OldHealth
    );
}

void UBaruMonsterAttributeSet::OnRep_MaxHealth(
    const FGameplayAttributeData& OldMaxHealth
)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(
        UBaruMonsterAttributeSet,
        MaxHealth,
        OldMaxHealth
    );
}

void UBaruMonsterAttributeSet::OnRep_Suppression(
    const FGameplayAttributeData& OldSuppression
)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(
        UBaruMonsterAttributeSet,
        Suppression,
        OldSuppression
    );
}

void UBaruMonsterAttributeSet::OnRep_MaxSuppression(
    const FGameplayAttributeData& OldMaxSuppression
)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(
        UBaruMonsterAttributeSet,
        MaxSuppression,
        OldMaxSuppression
    );
}

void UBaruMonsterAttributeSet::OnRep_PhysicalDefense(
    const FGameplayAttributeData& OldPhysicalDefense
)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(
        UBaruMonsterAttributeSet,
        PhysicalDefense,
        OldPhysicalDefense
    );
}