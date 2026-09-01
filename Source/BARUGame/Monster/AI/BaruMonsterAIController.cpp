


#include "Monster/AI/BaruMonsterAIController.h"

#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

#include "Monster/Characters/BaruMonsterCharacter.h"
#include "Monster/Data/BaruMonsterDataAsset.h"
#include "Monster/Components/BaruMonsterNavigationComponent.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "TimerManager.h"
#include "BaruLog.h"

namespace BaruMonsterBlackboardKeys
{
	const FName TargetActor(TEXT("TargetActor"));

	const FName LastKnownTargetLocation(
		TEXT("LastKnownTargetLocation")
	);

	const FName HasLastKnownTargetLocation(
		TEXT("HasLastKnownTargetLocation")
	);
}

ABaruMonsterAIController::ABaruMonsterAIController()
{
	// 몬스터가 사용할 감각 기관을 생성
	MonsterPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(
			TEXT("MonsterPerceptionComponent"));
	
	// 몬스터의 이동 요청과 설정을 관리하는 컴포넌트 생성
	MonsterNavigationComponent = CreateDefaultSubobject<UBaruMonsterNavigationComponent>(
			TEXT("MonsterNavigationComponent"));
	
	// AAIController에게 이 컴포넌트가 자신의 감각 기관 설정
	SetPerceptionComponent(*MonsterPerceptionComponent);
	
	// 시각 감지에 사용할 설정 객체를 생성
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(
			TEXT("SightConfig"));
	
	// 아직 팀 구분 시스템이 없으므로 모든 관계의 대상을 감지
	// 실제 플레이어 여부는 감지 이벤트에서 다시 검사할 예정
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	
	// 게임이 시작되기 전에 시각 감각을 감각 기관에 미리 등록
	MonsterPerceptionComponent->ConfigureSense(
		*SightConfig
	);

	// 여러 감각 중 시각을 기본 감각으로 지정
	MonsterPerceptionComponent->SetDominantSense(
		SightConfig->GetSenseImplementation()
	);
	
	// 현재 게임 기준 인원인 10명만큼 목록 공간을 미리 준비
	// 10명을 넘는다고 막히는 것은 아니며 필요하면 자동으로 늘어남
	VisiblePlayerCandidates.Reserve(10);
}


void ABaruMonsterAIController::OnPossess(APawn* InPawn)
{
	// 부모 AIController가 먼저 Pawn을 정상적으로 등록
	Super::OnPossess(InPawn);

	// AI 판단과 설정 적용은 서버에서만 실행
	if (!HasAuthority())
	{
		return;
	}

	// 지금은 감각 기관의 등록이 끝나지 않았을 수 있으므로
	// 다음 프레임에 시야 설정을 한 번 적용
	GetWorldTimerManager().SetTimerForNextTick(
		this,
		&ABaruMonsterAIController::InitializeFromControlledMonster
	);
}

void ABaruMonsterAIController::InitializeFromControlledMonster()
{
	// 다음 프레임 사이에 조종 대상이 바뀌었을 수도 있으므로
	// 현재 조종 중인 Pawn을 다시 가져옴
	const ABaruMonsterCharacter* MonsterCharacter =
		Cast<ABaruMonsterCharacter>(GetPawn());

	if (!IsValid(MonsterCharacter))
	{
		BARU_NET_LOG(
			this,
			LogBaruAI,
			Error,
			TEXT("Controlled Pawn is not a BaruMonsterCharacter.")
		);

		return;
	}

	// 몬스터 블루프린트에 지정된 설정표를 가져옴
	const UBaruMonsterDataAsset* MonsterDataAsset =
		MonsterCharacter->GetMonsterDataAsset();

	if (!IsValid(MonsterDataAsset))
	{
		BARU_NET_LOG(
			this,
			LogBaruAI,
			Error,
			TEXT("Monster DataAsset is not assigned.")
		);

		return;
	}

	// 감각 기관이 준비된 뒤 시야 거리와 시야각을 적용
	ApplySightSettings(*MonsterDataAsset);
	
	// 몬스터의 이동속도와 도착 허용 범위를 적용
	if (!IsValid(MonsterNavigationComponent))
	{
		BARU_NET_LOG(
			this,
			LogBaruAI,
			Error,
			TEXT(
				"Monster Navigation Component is invalid."
			)
		);

		return;
	}

	MonsterNavigationComponent->ApplyMovementSettings(
		*MonsterDataAsset
	);
	
	// 이 몬스터의 DataAsset에 지정된 Behavior Tree 확인
	if (!IsValid(MonsterDataAsset->BehaviorTreeAsset))
	{
		BARU_NET_LOG(
		   this,
		   LogBaruAI,
		   Error,
		   TEXT("Monster BehaviorTree is not assigned.")
		);

		return;
	}

	// Behavior Tree와 연결된 Blackboard를 초기화하고 실행
	if (!RunBehaviorTree(MonsterDataAsset->BehaviorTreeAsset))
	{
		BARU_NET_LOG(
		   this,
		   LogBaruAI,
		   Error,
		   TEXT("Failed to run Monster BehaviorTree.")
		);

		return;
	}
	
	// BT 실행에 성공한 뒤 초기 감지 상태를 반영
	UpdateBlackboardFromPerceptionState();

	BARU_NET_LOG(
	   this,
	   LogBaruAI,
	   Log,
	   TEXT("Monster BehaviorTree started: %s"),
	   *GetNameSafe(MonsterDataAsset->BehaviorTreeAsset)
	);
		
}

//시각정보세팅
void ABaruMonsterAIController::ApplySightSettings(
	const UBaruMonsterDataAsset& MonsterDataAsset
)
{
	// 플레이어를 처음 발견할 수 있는 거리를 적용
	SightConfig->SightRadius =
		MonsterDataAsset.SightRadius;

	// 이미 발견한 플레이어를 놓치게 되는 거리를 적용
	SightConfig->LoseSightRadius =
		MonsterDataAsset.LoseSightRadius;

	// 정면을 기준으로 한쪽 방향의 시야각을 적용
	SightConfig->PeripheralVisionAngleDegrees =
		MonsterDataAsset.PeripheralVisionAngle;

	// 음수 기억시간이 들어오지 않도록 보정하고 실행 중 보관
	SightMemoryDuration = FMath::Max(
		0.0f,
		MonsterDataAsset.SightMemoryDuration
	);

	// AI Perception 내부에도 같은 기억시간을 적용
	SightConfig->SetMaxAge(
		SightMemoryDuration
	);

	// 실행 중인 감각 시스템이 변경된 설정을 재설정
	MonsterPerceptionComponent->RequestStimuliListenerUpdate();

	BARU_NET_LOG(
		this,
		LogBaruAI,
		Log,
		TEXT(
			"Sight settings applied. "
			"Sight=%.1f, LoseSight=%.1f, HalfAngle=%.1f"
		),
		SightConfig->SightRadius,
		SightConfig->LoseSightRadius,
		SightConfig->PeripheralVisionAngleDegrees
	);
}

void ABaruMonsterAIController::BeginPlay()
{
	Super::BeginPlay();

	// 몬스터 감지 판단은 서버만 처리
	if (!HasAuthority())
	{
		return;
	}

	if (!IsValid(MonsterPerceptionComponent))
	{
		BARU_NET_LOG(
			this,
			LogBaruAI,
			Error,
			TEXT("Monster Perception Component is invalid.")
		);

		return;
	}

	// 감각 기관에서 대상의 감지 상태가 바뀌면
	// HandleTargetPerceptionUpdated 함수를 호출하도록 연결
	MonsterPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(
		this,
		&ABaruMonsterAIController::HandleTargetPerceptionUpdated
	);
}

void ABaruMonsterAIController::AddVisiblePlayerCandidate(
	APawn* PlayerPawn
)
{
	if (!IsValid(PlayerPawn))
	{
		return;
	}

	// 기존 목록에 남아 있는 무효한 플레이어부터 정리
	RemoveInvalidPlayerCandidates();

	// 이미 들어 있는 플레이어라면 중복으로 추가하지 않음
	VisiblePlayerCandidates.AddUnique(
		TWeakObjectPtr<APawn>(PlayerPawn)
	);
	
	SelectClosestVisiblePlayer();
}

void ABaruMonsterAIController::RemoveVisiblePlayerCandidate(
	APawn* PlayerPawn
)
{
	// 시야에서 놓친 플레이어와 이미 사라진 플레이어를 함께 제거
	VisiblePlayerCandidates.RemoveAll(
		[PlayerPawn](
			const TWeakObjectPtr<APawn>& Candidate
		)
		{
			return
				!Candidate.IsValid() ||
				Candidate.Get() == PlayerPawn;
		}
	);
	
	SelectClosestVisiblePlayer();
}

void ABaruMonsterAIController::RemoveInvalidPlayerCandidates()
{
	VisiblePlayerCandidates.RemoveAll(
		[](
			const TWeakObjectPtr<APawn>& Candidate
		)
		{
			return !Candidate.IsValid();
		}
	);
}

void ABaruMonsterAIController::HandleTargetPerceptionUpdated(
	AActor* Actor,
	FAIStimulus Stimulus
)
{
	// 몬스터의 감지 결과는 서버에서만 판단
	if (!HasAuthority())
	{
		return;
	}

	// 감지한 액터가 Pawn인지 확인
	APawn* SensedPawn = Cast<APawn>(Actor);

	if (!IsValid(SensedPawn))
	{
		return;
	}

	// 플레이어가 조종하는 Pawn만 처리
	// 다른 몬스터나 NPC는 무시
	if (!SensedPawn->IsPlayerControlled())
	{
		// 이전에는 플레이어였지만 현재 조종되지 않는 Pawn이
		// 목록에 남아 있을 가능성도 함께 정리
		RemoveVisiblePlayerCandidate(SensedPawn);
		
		// 남은 후보 상태를 블랙보드에 반영
		UpdateBlackboardFromPerceptionState();

		return;
	}

	// 플레이어를 현재 정상적으로 보고 있는 경우
	if (Stimulus.WasSuccessfullySensed())
	{
		// 현재 보이는 플레이어가 있으므로
		// 이전 수색에 사용하던 마지막 목격 기억을 제거
		ClearLastKnownTargetLocation();
		
		// 발견한 플레이어를 추적 후보 목록에 추가
		// 가장 가까운 플레이어를 현재 대상으로 선택
		AddVisiblePlayerCandidate(SensedPawn);
		
		// 새로 선택된 타깃을 블랙보드에 반영
		UpdateBlackboardFromPerceptionState();
		
		BARU_NET_LOG(
			this,
			LogBaruAI,
			Log,
			TEXT("Player detected: %s / "
			"Visible candidates: %d / "
			"Current target: %s"
			),
			*GetNameSafe(Actor),
			VisiblePlayerCandidates.Num(),
			*GetNameSafe(CurrentTarget.Get())
		);

		return;
	}
	
	// 후보 목록에서 제거하기 전에
	// 놓친 플레이어가 현재 추적 대상이었는지 기억
	const bool bLostCurrentTarget = CurrentTarget.Get() == SensedPawn;
	
	// 플레이어를 마지막으로 감지한 위치를 보관
	const FVector LostTargetLocation = Stimulus.StimulusLocation;
	
	// 시야에서 놓친 플레이어를 후보 목록에서 제거
	// 이 과정에서 남은 후보 중 새로운 대상이 선택될 수 있음
	RemoveVisiblePlayerCandidate(SensedPawn);

	// 이전에 발견한 플레이어를 시야에서 놓친 경우
	// 대신 추적할 다른 플레이어도 없는 경우에만 위치를 기억
	if (bLostCurrentTarget &&
		!IsValid(GetCurrentTarget()))
	{
		RememberLastKnownTargetLocation(
			LostTargetLocation
		);
	}
	
	// 남은 타깃 또는 마지막 목격 위치를 블랙보드에 반영
	UpdateBlackboardFromPerceptionState();
	
	BARU_NET_LOG(
		this,
		LogBaruAI,
		Log,
		TEXT(
			"Player lost: %s / "
			"Visible candidates: %d / "
			"Current target: %s / "
			"Last stimulus location: %s / "
			"Memory stored: %s"
		),
		*GetNameSafe(Actor),
		VisiblePlayerCandidates.Num(),
		*GetNameSafe(CurrentTarget.Get()),
		*LostTargetLocation.ToString(),
		bHasLastKnownTargetLocation
		? TEXT("true")
		: TEXT("false")
	);
}

APawn* ABaruMonsterAIController::GetCurrentTarget() const
{
	// 약한 참조가 무효라면 자동으로 nullptr 반환
	return CurrentTarget.Get();
}

bool ABaruMonsterAIController::HasLastKnownTargetLocation() const
{
	return bHasLastKnownTargetLocation;
}

FVector ABaruMonsterAIController::GetLastKnownTargetLocation() const
{
	return LastKnownTargetLocation;
}

void ABaruMonsterAIController::SelectClosestVisiblePlayer()
{
	APawn* ControlledPawn = GetPawn();

	if (!IsValid(ControlledPawn))
	{
		CurrentTarget.Reset();
		return;
	}

	APawn* ClosestPlayer = nullptr;
	float ClosestDistanceSquared =
		TNumericLimits<float>::Max();

	for (const TWeakObjectPtr<APawn>& Candidate :
		 VisiblePlayerCandidates)
	{
		APawn* CandidatePawn = Candidate.Get();

		if (!IsValid(CandidatePawn) ||
			!CandidatePawn->IsPlayerControlled())
		{
			continue;
		}

		const float DistanceSquared =
			FVector::DistSquared(
				ControlledPawn->GetActorLocation(),
				CandidatePawn->GetActorLocation()
			);

		if (DistanceSquared < ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;
			ClosestPlayer = CandidatePawn;
		}
	}

	// 기존 대상과 같으면 변경하지 않음
	if (CurrentTarget.Get() == ClosestPlayer)
	{
		return;
	}

	CurrentTarget = ClosestPlayer;

	BARU_NET_LOG(
		this,
		LogBaruAI,
		Log,
		TEXT("Current target changed: %s"),
		*GetNameSafe(CurrentTarget.Get())
	);
}

void ABaruMonsterAIController::RememberLastKnownTargetLocation(
	const FVector& TargetLocation
)
{
	// 기존에 실행 중인 기억 삭제 타이머가 있다면 먼저 취소
	GetWorldTimerManager().ClearTimer(
		SightMemoryTimerHandle
	);

	// 기억시간이 0이라면 마지막 위치를 보관하지 않음
	if (SightMemoryDuration <= 0.0f)
	{
		LastKnownTargetLocation = FVector::ZeroVector;
		bHasLastKnownTargetLocation = false;
		return;
	}

	// 플레이어를 마지막으로 확인한 위치를 저장
	LastKnownTargetLocation = TargetLocation;
	bHasLastKnownTargetLocation = true;

	// 기억시간이 지나면 마지막 목격 정보를 자동으로 제거
	GetWorldTimerManager().SetTimer(
		SightMemoryTimerHandle,
		this,
		&ABaruMonsterAIController::
			ClearLastKnownTargetLocation,
		SightMemoryDuration,
		false
	);

	BARU_NET_LOG(
		this,
		LogBaruAI,
		Log,
		TEXT(
			"Last known target location remembered: %s / "
			"Duration: %.1f"
		),
		*LastKnownTargetLocation.ToString(),
		SightMemoryDuration
	);
}

void ABaruMonsterAIController::ClearLastKnownTargetLocation()
{
	// 수동 삭제와 타이머 호출 모두 안전하게 처리
	GetWorldTimerManager().ClearTimer(
		SightMemoryTimerHandle
	);

	// 이미 기억이 없다면 추가 처리하지 않음
	if (!bHasLastKnownTargetLocation)
	{
		return;
	}

	LastKnownTargetLocation = FVector::ZeroVector;
	bHasLastKnownTargetLocation = false;
	
	// 기억이 끝난 상태를 블랙보드에도 반영
	UpdateBlackboardFromPerceptionState();

	BARU_NET_LOG(
		this,
		LogBaruAI,
		Log,
		TEXT("Last known target location forgotten.")
	);
			
}

void ABaruMonsterAIController::UpdateBlackboardFromPerceptionState()
{
	UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();

	if (!IsValid(BlackboardComponent))
	{
		BARU_NET_LOG(
			this,
			LogBaruAI,
			Error,
			TEXT("Monster Blackboard Component is invalid.")
		);

		return;
	}

	APawn* TargetPawn = CurrentTarget.Get();

	// 현재 보이는 추적 대상이 있는 경우
	if (IsValid(TargetPawn))
	{
		BlackboardComponent->SetValueAsObject(
			BaruMonsterBlackboardKeys::TargetActor,
			TargetPawn
		);

		// 추적 대상이 보이면 이전 목격 위치는 사용하지 않음
		BlackboardComponent->ClearValue(
			BaruMonsterBlackboardKeys::
				LastKnownTargetLocation
		);

		BlackboardComponent->SetValueAsBool(
			BaruMonsterBlackboardKeys::
				HasLastKnownTargetLocation,
			false
		);

		return;
	}

	// 현재 보이는 대상이 없다면 추적 대상 키를 비움
	BlackboardComponent->ClearValue(
		BaruMonsterBlackboardKeys::TargetActor
	);

	// 유효한 마지막 목격 위치가 있는 경우
	if (bHasLastKnownTargetLocation &&
		!LastKnownTargetLocation.ContainsNaN())
	{
		BlackboardComponent->SetValueAsVector(
			BaruMonsterBlackboardKeys::
				LastKnownTargetLocation,
			LastKnownTargetLocation
		);

		BlackboardComponent->SetValueAsBool(
			BaruMonsterBlackboardKeys::
				HasLastKnownTargetLocation,
			true
		);

		return;
	}

	// 현재 대상과 마지막 목격 위치가 모두 없는 상태
	BlackboardComponent->ClearValue(
		BaruMonsterBlackboardKeys::
			LastKnownTargetLocation
	);

	BlackboardComponent->SetValueAsBool(
		BaruMonsterBlackboardKeys::
			HasLastKnownTargetLocation,
		false
	);
}

void ABaruMonsterAIController::UpdateMovementFromPerceptionState()
{
	// 몬스터의 이동 판단은 서버에서만 수행
	if (!HasAuthority())
	{
		return;
	}

	if (!IsValid(MonsterNavigationComponent))
	{
		return;
	}

	// 현재 보이는 타깃이 있다면 계속 추적
	if (APawn* TargetPawn = GetCurrentTarget();
		IsValid(TargetPawn))
	{
		MonsterNavigationComponent->ChaseTarget(
			TargetPawn
		);

		return;
	}

	// 보이는 타깃은 없지만 마지막 목격 위치가 있다면
	// 해당 위치까지 이동해 플레이어를 수색
	if (bHasLastKnownTargetLocation)
	{
		MonsterNavigationComponent->MoveToLocation(
			LastKnownTargetLocation
		);

		return;
	}

	// 추적 대상과 마지막 목격 위치가 모두 없다면 이동 중단
	MonsterNavigationComponent->StopMovement();
}


