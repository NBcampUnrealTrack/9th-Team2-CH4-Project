
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaruMonsterNavigationComponent.generated.h"

class UBaruMonsterDataAsset;
class APawn;


UCLASS(ClassGroup = (Monster))
class BARUGAME_API UBaruMonsterNavigationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBaruMonsterNavigationComponent();
	
	// DataAsset의 이동 설정을 실행 중 값으로 저장
	void ApplyMovementSettings(
		const UBaruMonsterDataAsset& MonsterDataAsset
	);

	float GetPatrolSpeed() const
	{
		return PatrolSpeed;
	}

	float GetChaseSpeed() const
	{
		return ChaseSpeed;
	}

	float GetMoveAcceptanceRadius() const
	{
		return MoveAcceptanceRadius;
	}
	
	// 지정한 플레이어를 추적
	// 이동 요청에 성공하면 true 반환
	bool ChaseTarget(APawn* TargetPawn);
	
	// 지정한 월드 위치로 이동
	// 이동 요청에 성공하면 true 반환
	bool MoveToLocation(const FVector& TargetLocation);
	
	// 현재 진행 중인 이동 요청을 중단
	void StopMovement();
	
	// CoreAttributeSet의 기본 이동속도를 변경
	void SetControlledMonsterMoveSpeed(float NewBaseMoveSpeed);

private:
	
	// 평상시 배회 속도
	float PatrolSpeed = 0.0f;

	// 플레이어 추적 속도
	float ChaseSpeed = 0.0f;

	// 목적지에 도착했다고 판단할 허용 거리
	float MoveAcceptanceRadius = 0.0f;

};
