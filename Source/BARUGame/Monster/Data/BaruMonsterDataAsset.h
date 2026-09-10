

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BaruMonsterDataAsset.generated.h"


class UBehaviorTree;
class UGameplayAbility;

/**
 * 몬스터마다 다른 설정값을 보관하는 데이터 에셋
 */
UCLASS(BlueprintType)
class BARUGAME_API UBaruMonsterDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	
	 // =========================================================================
    // AI
    // =========================================================================

    // 이 몬스터가 판단에 사용할 Behavior Tree
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|AI"
    )
    TObjectPtr<UBehaviorTree> BehaviorTreeAsset;


    // =========================================================================
    // Perception
    // =========================================================================

    // 플레이어를 처음 발견할 수 있는 거리
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|Perception",
        meta = (ClampMin = "0.0")
    )
    float SightRadius = 0.0f;

    // 플레이어를 발견한 뒤 놓치지 않고 볼 수 있는 거리
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|Perception",
        meta = (ClampMin = "0.0")
    )
    float LoseSightRadius = 0.0f;

    // 몬스터가 좌우로 볼 수 있는 시야각
    // 정면을 기준으로 한쪽 방향의 시야각
    // 60으로 설정하면 왼쪽 60도 + 오른쪽 60도로 총 120도를 볼 수 있음
    // 180이면 뒤까지 포함한 전체 360도를 볼 수 있음
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|Perception",
        meta = (
            ClampMin = "0.0",
            ClampMax = "180.0",
            Units = "Degrees"
        )
    )
    float PeripheralVisionAngle = 0.0f;

    // 플레이어가 시야에서 사라진 뒤 위치를 기억하는 시간
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|Perception",
        meta = (ClampMin = "0.0")
    )
    float SightMemoryDuration = 0.0f;


    // =========================================================================
    // Movement
    // =========================================================================

    // 평소 배회할 때의 이동 속도
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|Movement",
        meta = (ClampMin = "0.0")
    )
    float PatrolSpeed = 0.0f;

    // 플레이어를 추적할 때의 이동 속도
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|Movement",
        meta = (ClampMin = "0.0")
    )
    float ChaseSpeed = 0.0f;

    // 목적지에 이 정도로 가까워지면 도착했다고 판단
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|Movement",
        meta = (ClampMin = "0.0")
    )
    float MoveAcceptanceRadius = 0.0f;


    // =========================================================================
    // Stats
    // =========================================================================

    // 몬스터의 최대 체력
    // 게임 시작 시 현재 체력도 이 값으로 채움
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|Stats",
        meta = (ClampMin = "1.0")
    )
    float MaxHealth = 100.0f;

    // 몬스터의 물리 방어력
    // 값이 높을수록 물리 피해가 감소
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|Stats",
        meta = (ClampMin = "0.0")
    )
    float PhysicalDefense = 0.0f;

    // 몬스터의 특수 피해 저항률
    // 0.0은 저항 없음, 1.0은 특수 피해 100% 저항
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|Stats",
        meta = (
            ClampMin = "0.0",
            ClampMax = "1.0"
        )
    )
    float SpecialResistance = 0.0f;

    // 몬스터가 버틸 수 있는 최대 제압도
    // 게임 시작 시 현재 제압도도 이 값으로 채움
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|Stats",
        meta = (ClampMin = "1.0")
    )
    float MaxSuppression = 100.0f;


    // =========================================================================
    // Combat
    // =========================================================================

    // 이 몬스터가 사용할 공격 Gameplay Ability
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|Combat"
    )
    TSubclassOf<UGameplayAbility> AttackAbilityClass;

    // 이 거리 안에 플레이어가 들어오면 공격
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|Combat",
        meta = (ClampMin = "0.0")
    )
    float AttackRange = 0.0f;

    // 공격한 뒤 다음 공격까지 기다리는 시간
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|Combat",
        meta = (ClampMin = "0.0")
    )
    float AttackCooldown = 0.0f;
	
};

