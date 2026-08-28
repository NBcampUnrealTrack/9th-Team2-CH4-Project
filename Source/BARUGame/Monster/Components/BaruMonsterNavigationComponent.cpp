

#include "BaruMonsterNavigationComponent.h"
#include "Monster/Data/BaruMonsterDataAsset.h"
#include "Monster/Characters/BaruMonsterCharacter.h"

#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/BaruCoreAttributeSet.h"
#include "Navigation/PathFollowingComponent.h"

#include "BaruLog.h"


UBaruMonsterNavigationComponent::UBaruMonsterNavigationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UBaruMonsterNavigationComponent::ApplyMovementSettings(
	const UBaruMonsterDataAsset& MonsterDataAsset)
{
	// 잘못된 음수 값이 실행 중 사용되지 않도록 보정
	PatrolSpeed = FMath::Max(
		0.0f,
		MonsterDataAsset.PatrolSpeed
	);

	ChaseSpeed = FMath::Max(
		0.0f,
		MonsterDataAsset.ChaseSpeed
	);

	MoveAcceptanceRadius = FMath::Max(
		0.0f,
		MonsterDataAsset.MoveAcceptanceRadius
	);
	
	// 초기 이동 상태는 배회 속도로 설정
	SetControlledMonsterMoveSpeed(PatrolSpeed);
}

void UBaruMonsterNavigationComponent::SetControlledMonsterMoveSpeed(float NewBaseMoveSpeed)
{
	// 이 컴포넌트를 소유한 AIController를 가져옴
	AAIController* OwnerController =
		Cast<AAIController>(GetOwner());

	if (!IsValid(OwnerController) ||
		!OwnerController->HasAuthority())
	{
		return;
	}

	ABaruMonsterCharacter* MonsterCharacter =
		Cast<ABaruMonsterCharacter>(
			OwnerController->GetPawn()
		);

	if (!IsValid(MonsterCharacter))
	{
		return;
	}

	UAbilitySystemComponent* ASC =
		MonsterCharacter->GetAbilitySystemComponent();

	if (!IsValid(ASC))
	{
		BARU_NET_LOG(
			OwnerController,
			LogBaruGAS,
			Error,
			TEXT("Monster ASC is invalid.")
		);

		return;
	}

	const float SafeMoveSpeed =
		FMath::Max(0.0f, NewBaseMoveSpeed);

	// Core MoveSpeed의 기본값을 변경
	// 활성화된 감속·가속 GameplayEffect는 유지
	ASC->SetNumericAttributeBase(
		UBaruCoreAttributeSet::GetMoveSpeedAttribute(),
		SafeMoveSpeed
	);
}

bool UBaruMonsterNavigationComponent::ChaseTarget(APawn* TargetPawn)
{
	// 이 컴포넌트를 소유한 AIController를 가져옴
	AAIController* OwnerController = Cast<AAIController>(GetOwner());

	// 이동 판단과 요청은 서버에서만 수행
	if (!IsValid(OwnerController) ||
		!OwnerController->HasAuthority())
	{
		return false;
	}

	// 파괴됐거나 존재하지 않는 타깃은 추적하지 않음
	if (!IsValid(TargetPawn))
	{
		return false;
	}

	// 추적하기 전에 Core MoveSpeed를 추적 속도로 변경
	SetControlledMonsterMoveSpeed(ChaseSpeed);
	
	// NavMesh를 이용해 움직이는 플레이어를 계속 따라가도록 요청
	const EPathFollowingRequestResult::Type MoveResult =
		OwnerController->MoveToActor(
			TargetPawn,
			MoveAcceptanceRadius
		);

	// 이동 가능한 경로를 만들지 못했다면 요청 실패
	if (MoveResult ==
		EPathFollowingRequestResult::Failed)
	{
		BARU_NET_LOG(
			OwnerController,
			LogBaruAI,
			Warning,
			TEXT(
				"Failed to request movement "
				"toward target: %s"
			),
			*GetNameSafe(TargetPawn)
		);

		return false;
	}
	
	// 이동 요청 성공 또는 이미 목표 범위 안에 있음
	return true;
}

bool UBaruMonsterNavigationComponent::MoveToLocation(
	const FVector& TargetLocation
)
{
	// 이 컴포넌트를 소유한 AIController를 가져옴
	AAIController* OwnerController =
		Cast<AAIController>(GetOwner());

	// 몬스터의 이동 판단과 경로 요청은 서버에서만 수행
	if (!IsValid(OwnerController) ||
		!OwnerController->HasAuthority())
	{
		return false;
	}

	// 잘못된 좌표가 전달됐다면 이동하지 않음
	if (TargetLocation.ContainsNaN())
	{
		return false;
	}

	// 마지막 목격 위치까지는 추적 속도로 이동
	SetControlledMonsterMoveSpeed(ChaseSpeed);

	// NavMesh를 이용해 지정된 위치로 이동하도록 요청
	const EPathFollowingRequestResult::Type MoveResult =
		OwnerController->MoveToLocation(
			TargetLocation,
			MoveAcceptanceRadius
		);

	// 이동 가능한 경로를 만들지 못했다면 요청 실패
	if (MoveResult ==
		EPathFollowingRequestResult::Failed)
	{
		BARU_NET_LOG(
			OwnerController,
			LogBaruAI,
			Warning,
			TEXT(
				"Failed to request movement "
				"toward location: %s"
			),
			*TargetLocation.ToString()
		);

		return false;
	}

	// 이동 요청 성공 또는 이미 목표 범위 안에 있음
	return true;
}

void UBaruMonsterNavigationComponent::StopMovement()
{
	// 이 컴포넌트를 소유한 AIController를 가져옴
	AAIController* OwnerController =
		Cast<AAIController>(GetOwner());

	// 몬스터의 이동 중단은 서버에서만 수행
	if (!IsValid(OwnerController) ||
		!OwnerController->HasAuthority())
	{
		return;
	}

	// 현재 진행 중인 MoveToActor 또는
	// MoveToLocation 요청을 중단
	OwnerController->StopMovement();
}