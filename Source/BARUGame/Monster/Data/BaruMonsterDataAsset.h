

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
    // Ambush
    // =========================================================================

    // 이 몬스터 종류가 디렉터의 매복 명령을 받을 수 있는지 결정
    //
    // false:
    // 매복 대상에서 제외하고 기존 추적·조사 행동만 사용
    //
    // true:
    // 디렉터가 상황에 따라 이 몬스터를 매복 담당으로 선택할 수 있음
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|AI|Ambush"
    )
    bool bCanAmbush = false;
    
    // 매복 위치에서 기다리다가 목표가 이 거리 안으로 들어오면 기습 시작
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|AI|Ambush",
        meta = (ClampMin = "0.0", Units = "cm")
    )
    float AmbushTriggerDistance = 400.0f;

    // 목표가 접근하지 않을 때 매복을 유지할 최대 시간
    //
    // 시간이 지나면 매복을 취소하고 기존 AI 판단으로 복귀
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|AI|Ambush",
        meta = (ClampMin = "0.1", Units = "s")
    )
    float AmbushMaximumWaitDuration = 15.0f;
    
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
    
    // 그로기 상태가 유지되는 시간
    // 시간이 끝나면 살아 있는 몬스터의 제압도를 회복하고 행동을 재개
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|Combat",
        meta = (ClampMin = "0.1", Units = "s")
    )
    float GroggyDuration = 5.0f;
    
    // 피격 시 공격자의 반대 방향으로 밀리는 수평 속도
    // 이동 거리가 아니라 속도이며, 0이면 피격 밀림을 사용하지 않음
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|Combat",
        meta = (ClampMin = "0.0", Units = "cm/s")
    )
    float HitPushSpeed = 200.0f;
    
    //----------------
    // 위협도 / 어그로
    //----------------

    // 이 간격마다 대상 점수를 다시 비교
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|AI|Threat",
        meta = (ClampMin = "0.1", Units = "s")
    )
    float ThreatUpdateInterval = 0.5f;

    // 거리 점수가 0이 되는 기준 거리
    // 실제 감지 가능 거리는 기존 시야 설정을 따름
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|AI|Threat",
        meta = (ClampMin = "1.0", Units = "cm")
    )
    float ThreatDistanceReference = 2000.0f;

    // 바로 가까이에 있는 플레이어가 받는 최대 거리 점수
    // 기준 거리까지 멀어질수록 점수가 0으로 감소
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|AI|Threat",
        meta = (ClampMin = "0.0")
    )
    float ThreatDistanceWeight = 30.0f;

    // 실제 받은 체력 피해 1당 공격자에게 추가할 위협도
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|AI|Threat",
        meta = (ClampMin = "0.0")
    )
    float ThreatPerDamage = 1.0f;

    // 플레이어 한 명에게 쌓이는 피해 위협도의 상한
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|AI|Threat",
        meta = (ClampMin = "0.0")
    )
    float MaxDamageThreat = 100.0f;

    // 누적 피해 위협도가 초당 감소하는 양
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|AI|Threat",
        meta = (ClampMin = "0.0")
    )
    float ThreatDecayPerSecond = 5.0f;

    // 현재 추적 대상에게 추가하는 유지 점수
    // 점수 차이가 작을 때 대상을 계속 바꾸는 현상을 줄임
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|AI|Threat",
        meta = (ClampMin = "0.0")
    )
    float CurrentTargetThreatBonus = 15.0f;
    
    // 전투 중 시야 판정이 잠깐 끊겨도
    // 벽으로 가려지지 않았다면 현재 대상을 계속 추적할 거리
    //
    // 이 거리 밖으로 벗어나면 일반적인 마지막 목격 위치 수색으로 전환
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|AI|Combat Target Retention",
        meta = (ClampMin = "0.0", Units = "cm")
    )
    float CombatTargetRetentionDistance = 800.0f;

    // 전투 대상과 몬스터 사이에 실제 벽이 생겼는지
    // 다시 확인하는 시간 간격
    UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "Monster|AI|Combat Target Retention",
        meta = (ClampMin = "0.05", Units = "s")
    )
    float CombatTargetRetentionCheckInterval = 0.25f;
	
};

