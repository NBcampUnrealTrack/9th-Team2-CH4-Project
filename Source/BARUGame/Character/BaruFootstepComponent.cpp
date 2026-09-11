// BaruFootstepComponent.cpp
#include "Character/BaruFootstepComponent.h"
#include "Character/BaruCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Interfaces/CombatInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Perception/AISense_Hearing.h"
#include "Engine/World.h"

UBaruFootstepComponent::UBaruFootstepComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// 소리만 내는 컴포넌트라 복제가 필요 없습니다. 각 클라가 알아서 재생합니다.
	SetIsReplicatedByDefault(false);
}

void UBaruFootstepComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<ACharacter>(GetOwner());
}

void UBaruFootstepComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerCharacter)
	{
		return;
	}

	const UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	// 사망·다운 중에는 발소리 없음. 소생 직후 착지음이 나지 않도록 상태도 초기화합니다.
	if (IsOwnerIncapacitated())
	{
		DistanceSinceLastStep = 0.0f;
		AirTime = 0.0f;
		bWasOnGround = true;
		return;
	}

	// 공중: 체공 시간만 잽니다.
	// IsMovingOnGround 는 MovementMode 기반이라 다른 플레이어 캐릭터에서도 정확합니다.
	if (!MoveComp->IsMovingOnGround())
	{
		AirTime += DeltaTime;
		bWasOnGround = false;
		return;
	}

	float Stride = WalkStrideLength;
	float Volume = WalkVolume;
	GetStateParams(Stride, Volume);

	// 착지: 방금 땅에 닿았고 충분히 떠 있었으면 한 번 크게.
	if (!bWasOnGround)
	{
		bWasOnGround = true;
		if (AirTime >= MinAirTimeForLandingSound)
		{
			PlayFootstep(FMath::Min(Volume * LandingVolumeScale, 1.0f));
			DistanceSinceLastStep = 0.0f;
		}
		AirTime = 0.0f;
	}

	// 걷기: 수평 이동 거리를 누적해서 보폭마다 한 번 재생합니다.
	const FVector Velocity = OwnerCharacter->GetVelocity();
	const float HorizontalSpeed = FVector(Velocity.X, Velocity.Y, 0.0f).Size();

	if (HorizontalSpeed < MinSpeedForFootstep)
	{
		// 멈춰 있으면 다음 출발 때 첫 발소리가 반 걸음 만에 나오도록 맞춰둡니다.
		DistanceSinceLastStep = Stride * 0.5f;
		return;
	}

	DistanceSinceLastStep += HorizontalSpeed * DeltaTime;
	if (DistanceSinceLastStep >= Stride)
	{
		// 프레임이 튀어도 한 틱에 한 번만 재생되도록 나머지만 남깁니다.
		DistanceSinceLastStep = FMath::Fmod(DistanceSinceLastStep, Stride);
		PlayFootstep(Volume);
	}
}

void UBaruFootstepComponent::PlayFootstep(float VolumeScale)
{
	FHitResult FloorHit;
	if (!TraceFloor(FloorHit))
	{
		return;
	}

	const FVector StepLocation = FloorHit.ImpactPoint;

	// 데디케이티드 서버는 들을 사람이 없어서 건너뜁니다. (리슨 서버 호스트는 재생)
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (USoundBase* Sound = SelectFootstepSound())
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				Sound,
				StepLocation,
				FRotator::ZeroRotator,
				VolumeScale,
				FMath::FRandRange(0.95f, 1.05f),   // 피치를 살짝 흔들어 반복감 완화
				0.0f,
				FootstepAttenuation);
		}
	}

	// AI 는 서버에서만 돌기 때문에 소음 보고도 서버에서만 합니다.
	// 앉아서 걸으면 Loudness 가 작아져서 몬스터가 덜 듣게 됩니다.
	if (bReportAINoise && GetOwner() && GetOwner()->HasAuthority())
	{
		UAISense_Hearing::ReportNoiseEvent(
			this, StepLocation, VolumeScale, GetOwner(), AINoiseRange, FName(TEXT("Footstep")));
	}
}

bool UBaruFootstepComponent::IsOwnerIncapacitated() const
{
	if (OwnerCharacter && OwnerCharacter->Implements<UCombatInterface>())
	{
		return ICombatInterface::Execute_IsDead(OwnerCharacter)
			|| ICombatInterface::Execute_IsDBNO(OwnerCharacter);
	}
	return false;
}

void UBaruFootstepComponent::GetStateParams(float& OutStride, float& OutVolume) const
{
	OutStride = WalkStrideLength;
	OutVolume = WalkVolume;

	if (!OwnerCharacter)
	{
		return;
	}

	// bIsCrouched 는 ACharacter 가 복제하는 값이라 다른 클라에서도 정확합니다.
	if (OwnerCharacter->bIsCrouched)
	{
		OutStride = CrouchStrideLength;
		OutVolume = CrouchVolume;
		return;
	}

	if (const ABaruCharacter* BaruChar = Cast<ABaruCharacter>(OwnerCharacter))
	{
		if (BaruChar->IsSprinting())
		{
			OutStride = SprintStrideLength;
			OutVolume = SprintVolume;
		}
	}
}

bool UBaruFootstepComponent::TraceFloor(FHitResult& OutHit) const
{
	UWorld* World = GetWorld();
	if (!World || !OwnerCharacter)
	{
		return false;
	}

	const UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent();
	const float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 88.0f;

	const FVector Start = OwnerCharacter->GetActorLocation();
	const FVector End = Start - FVector(0.0f, 0.0f, HalfHeight + 30.0f);

	FCollisionQueryParams Params(FName(TEXT("BaruFootstepTrace")), false, OwnerCharacter);
	Params.bReturnPhysicalMaterial = true;   // 나중에 표면별 발소리를 붙일 때 씁니다.

	return World->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, Params);
}

USoundBase* UBaruFootstepComponent::SelectFootstepSound()
{
	const int32 Num = DefaultFootstepSounds.Num();
	if (Num == 0)
	{
		return nullptr;
	}
	if (Num == 1)
	{
		return DefaultFootstepSounds[0];
	}

	// 같은 소리가 연속으로 나오지 않게 직전 인덱스는 건너뜁니다.
	int32 Index = 0;
	if (LastSoundIndex == INDEX_NONE)
	{
		Index = FMath::RandHelper(Num);
	}
	else
	{
		Index = FMath::RandHelper(Num - 1);
		if (Index >= LastSoundIndex)
		{
			++Index;
		}
	}

	LastSoundIndex = Index;
	return DefaultFootstepSounds[Index];
}