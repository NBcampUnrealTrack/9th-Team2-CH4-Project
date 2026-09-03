#include "Animation/Character/BaruAnimInstance.h"
#include "Character/BaruCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interfaces/CombatInterface.h"

void UBaruAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwnerCharacter = Cast<ABaruCharacter>(TryGetPawnOwner());
	if (OwnerCharacter)
	{
		OwnerMovement = OwnerCharacter->GetCharacterMovement();
	}
}

void UBaruAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 에디터 프리뷰나 폰이 아직 안 붙은 프레임 방어.
	// NativeInitializeAnimation 시점엔 폰이 없을 수 있어 여기서 한 번 더 시도합니다.
	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ABaruCharacter>(TryGetPawnOwner());
		if (!OwnerCharacter)
		{
			return;
		}
		OwnerMovement = OwnerCharacter->GetCharacterMovement();
	}

	if (!OwnerMovement)
	{
		return;
	}

	const FVector Velocity = OwnerCharacter->GetVelocity();
	const FVector GroundVelocity(Velocity.X, Velocity.Y, 0.f);

	GroundSpeed = GroundVelocity.Size();
	bIsInAir = OwnerMovement->IsFalling();
	bIsAccelerating = OwnerMovement->GetCurrentAcceleration().SizeSquared() > 0.f;
	Direction = CalculateDirection(GroundVelocity, OwnerCharacter->GetActorRotation());

	// 사망 여부는 CombatInterface 로 조회합니다.
	// _Implementation 을 직접 부르지 않고 Execute_ 를 쓰는 이유:
	// 나중에 BP 에서 오버라이드해도 그 결과가 반영되기 때문입니다.
	if (OwnerCharacter->Implements<UCombatInterface>())
	{
		bIsDead = ICombatInterface::Execute_IsDead(OwnerCharacter);
	}

	// GetBaseAimRotation 은 시뮬레이션 프록시(남의 캐릭터)에서도
	// 복제된 컨트롤 로테이션을 반환하므로 다른 플레이어 조준 방향도 맞게 나옵니다.
	AimPitch = FRotator::NormalizeAxis(OwnerCharacter->GetBaseAimRotation().Pitch);
}