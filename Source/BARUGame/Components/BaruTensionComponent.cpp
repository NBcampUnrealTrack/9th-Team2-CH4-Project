#include "Components/BaruTensionComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "GameFramework/Pawn.h"
#include "Player/BaruPlayerState.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/BaruPlayerAttributeSet.h"
#include "Monster/Characters/BaruMonsterCharacter.h"
#include "Interfaces/CombatInterface.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"
#include "BaruLog.h"

UBaruTensionComponent::UBaruTensionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	HeartbeatAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("HeartbeatAudioComponent"));
	HeartbeatAudioComponent->bAutoActivate = false;
	HeartbeatAudioComponent->bStopWhenOwnerDestroyed = true;
}

void UBaruTensionComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	// 1. [Server] 몬스터 근접 체크 주기적 실행
	if (OwnerActor->HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(
			ServerCheckTimerHandle,
			this,
			&UBaruTensionComponent::UpdateTensionOnServer,
			ServerCheckInterval,
			true
		);
	}

	// 2. [Client] 로컬 플레이어일 때만 심장소리 컴포넌트 세팅
	APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	if (OwnerPawn && OwnerPawn->IsLocallyControlled())
	{
		if (HeartbeatAudioComponent)
		{
			HeartbeatAudioComponent->AttachToComponent(OwnerActor->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			if (HeartbeatSound)
			{
				HeartbeatAudioComponent->SetSound(HeartbeatSound);
			}
			HeartbeatAudioComponent->SetVolumeMultiplier(0.0f);
			HeartbeatAudioComponent->SetPitchMultiplier(MinHeartbeatPitch);
		}

		TryBindToPlayerState();
	}
	else
	{
		// 서버나 원격 프록시에서는 틱 비활성화 (최적화)
		SetComponentTickEnabled(false);
	}
}

void UBaruTensionComponent::TryBindToPlayerState()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	ABaruPlayerState* PS = OwnerPawn->GetPlayerState<ABaruPlayerState>();
	if (PS)
	{
		CachedPlayerState = PS;
		PS->OnTensionChanged.AddUniqueDynamic(this, &UBaruTensionComponent::HandleTensionChanged);

		// 초기 수치 즉시 반영
		HandleTensionChanged(PS->GetPlayerAttributeSet() ? PS->GetPlayerAttributeSet()->GetTension() : 0.0f);
		GetWorld()->GetTimerManager().ClearTimer(ClientBindRetryTimerHandle);
	}
	else
	{
		// PlayerState가 복제될 때까지 0.1초마다 재시도
		GetWorld()->GetTimerManager().SetTimer(
			ClientBindRetryTimerHandle,
			this,
			&UBaruTensionComponent::TryBindToPlayerState,
			0.1f,
			false
		);
	}
}

void UBaruTensionComponent::HandleTensionChanged(float NewTension)
{
	// 0 이하(사망/소생/초기화) 시 보간 없이 즉각 사운드 컷
	if (NewTension <= 0.0f)
	{
		TargetVolume = 0.0f;
		CurrentVolume = 0.0f;
		TargetPitch = MinHeartbeatPitch;
		CurrentPitch = MinHeartbeatPitch;

		if (HeartbeatAudioComponent && HeartbeatAudioComponent->IsPlaying())
		{
			HeartbeatAudioComponent->Stop();
		}
		return;
	}

	// 최소 감지치 이하일 때는 서서히 페이드아웃
	if (NewTension <= SoundThresholdTension)
	{
		TargetVolume = 0.0f;
		TargetPitch = MinHeartbeatPitch;
		return;
	}

	// 15 ~ 100 사이의 정규화 비율 (0.0 ~ 1.0)
	const float Ratio = FMath::Clamp((NewTension - SoundThresholdTension) / (100.0f - SoundThresholdTension), 0.0f, 1.0f);

	TargetVolume = FMath::Lerp(0.15f, MaxHeartbeatVolume, Ratio);
	TargetPitch = FMath::Lerp(MinHeartbeatPitch, MaxHeartbeatPitch, Ratio);
}

void UBaruTensionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled() || !HeartbeatAudioComponent)
	{
		return;
	}

	// 볼륨과 피치를 부드럽게 보간
	CurrentVolume = FMath::FInterpTo(CurrentVolume, TargetVolume, DeltaTime, AudioInterpSpeed);
	CurrentPitch = FMath::FInterpTo(CurrentPitch, TargetPitch, DeltaTime, AudioInterpSpeed);

	HeartbeatAudioComponent->SetVolumeMultiplier(CurrentVolume);
	HeartbeatAudioComponent->SetPitchMultiplier(CurrentPitch);

	if (CurrentVolume > 0.01f && !HeartbeatAudioComponent->IsPlaying())
	{
		HeartbeatAudioComponent->Play();
	}
	else if (CurrentVolume <= 0.01f && HeartbeatAudioComponent->IsPlaying())
	{
		HeartbeatAudioComponent->Stop();
	}
}

void UBaruTensionComponent::UpdateTensionOnServer()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	if (!OwnerPawn)
	{
		return;
	}

	// 플레이어가 사망/다운 상태면 긴장도 갱신 중단
	ABaruPlayerState* PS = OwnerPawn->GetPlayerState<ABaruPlayerState>();
	if (!PS || PS->IsDead() || PS->IsDBNO())
	{
		return;
	}

	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	const UBaruPlayerAttributeSet* PlayerSet = PS->GetPlayerAttributeSet();
	if (!ASC || !PlayerSet)
	{
		return;
	}

	// 주변 몬스터 Overlap 탐색
	const FVector PlayerLoc = OwnerPawn->GetActorLocation();
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(ProximityRadius);
	FCollisionQueryParams QueryParams(TEXT("TensionProximityCheck"), false, OwnerActor);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByChannel(Overlaps, PlayerLoc, FQuat::Identity, ECC_Pawn, SphereShape, QueryParams);

	float ClosestDistSq = FMath::Square(ProximityRadius);
	bool bFoundAliveMonster = false;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (HitActor && HitActor->IsA<ABaruMonsterCharacter>())
		{
			if (ICombatInterface* Combat = Cast<ICombatInterface>(HitActor))
			{
				if (!Combat->Execute_IsDead(HitActor))
				{
					const float DistSq = FVector::DistSquared(PlayerLoc, HitActor->GetActorLocation());
					if (DistSq < ClosestDistSq)
					{
						ClosestDistSq = DistSq;
						bFoundAliveMonster = true;
					}
				}
			}
		}
	}

	const float CurrentTension = PlayerSet->GetTension();
	float NewTension = CurrentTension;

	if (bFoundAliveMonster)
	{
		// 거리가 가까울수록 가파르게 상승 (0.0 ~ 1.0)
		const float ProximityRatio = 1.0f - (FMath::Sqrt(ClosestDistSq) / ProximityRadius);
		const float GainAmount = MaxTensionGainRate * ProximityRatio * ServerCheckInterval;
		NewTension = FMath::Clamp(CurrentTension + GainAmount, 0.0f, PlayerSet->GetMaxTension());
	}
	else if (CurrentTension > 0.0f)
	{
		// 주변에 몬스터가 없으면 서서히 감소
		const float DecayAmount = TensionDecayRate * ServerCheckInterval;
		NewTension = FMath::Max(0.0f, CurrentTension - DecayAmount);
	}

	if (!FMath::IsNearlyEqual(CurrentTension, NewTension))
	{
		ASC->SetNumericAttributeBase(UBaruPlayerAttributeSet::GetTensionAttribute(), NewTension);
	}
}

void UBaruTensionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ServerCheckTimerHandle);
		World->GetTimerManager().ClearTimer(ClientBindRetryTimerHandle);
	}

	if (CachedPlayerState.IsValid())
	{
		CachedPlayerState->OnTensionChanged.RemoveDynamic(this, &UBaruTensionComponent::HandleTensionChanged);
	}

	Super::EndPlay(EndPlayReason);
}