


#include "Monster/AI/BaruMonsterAIController.h"

#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Monster/Characters/BaruMonsterCharacter.h"
#include "Monster/Data/BaruMonsterDataAsset.h"
#include "TimerManager.h"
#include "BaruLog.h"


ABaruMonsterAIController::ABaruMonsterAIController()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// 몬스터가 사용할 감각 기관을 생성
	MonsterPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(
			TEXT("MonsterPerceptionComponent"));
	
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
	
	// 현재 게임 기준 인원인 5명만큼 목록 공간을 미리 준비
	// 5명을 넘는다고 막히는 것은 아니며 필요하면 자동으로 늘어남
	VisiblePlayerCandidates.Reserve(5);
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
		&ABaruMonsterAIController::InitializeSightFromControlledMonster
	);
}

void ABaruMonsterAIController::InitializeSightFromControlledMonster()
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

	// 시야에서 사라진 대상을 기억하는 시간을 적용
	SightConfig->SetMaxAge(
		MonsterDataAsset.SightMemoryDuration
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
		return;
	}

	// 플레이어를 현재 정상적으로 보고 있는 경우
	if (Stimulus.WasSuccessfullySensed())
	{
		// 발견한 플레이어를 추적 후보 목록에 추가
		AddVisiblePlayerCandidate(SensedPawn);
		
		BARU_NET_LOG(
			this,
			LogBaruAI,
			Log,
			TEXT("Player detected: %s / "
			"Visible candidates: %d"),
			*GetNameSafe(Actor),
			VisiblePlayerCandidates.Num()
		);

		return;
	}
	
	// 시야에서 놓친 플레이어를 후보 목록에서 제거
	RemoveVisiblePlayerCandidate(SensedPawn);

	// 이전에 발견한 플레이어를 시야에서 놓친 경우
	BARU_NET_LOG(
		this,
		LogBaruAI,
		Log,
		TEXT(
			"Player lost: %s / "
			"Visible candidates: %d / "
			"Last known location: %s"
		),
		*GetNameSafe(Actor),
		VisiblePlayerCandidates.Num(),
		*Stimulus.StimulusLocation.ToString()
	);
}