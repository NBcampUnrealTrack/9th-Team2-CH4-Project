


#include "Monster/AI/BaruMonsterAIController.h"

#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

#include "Monster/Characters/BaruMonsterCharacter.h"
#include "Monster/AI/BaruMonsterTacticalRoute.h"
#include "Monster/Data/BaruMonsterDataAsset.h"
#include "Monster/Components/BaruMonsterNavigationComponent.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/World.h"
#include "GameplayTags/BaruGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Interfaces/CombatInterface.h"
#include "Player/BaruPlayerState.h"
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
    
    const FName MoveAcceptanceRadius(
       TEXT("MoveAcceptanceRadius")
    );
    
    // 디렉터의 이동·조사 명령이 있는지
    const FName HasDirectorInvestigation(
       TEXT("HasDirectorInvestigation")
    );

    // 디렉터가 지정한 조사 위치
    const FName DirectorTargetLocation(
       TEXT("DirectorTargetLocation")
    );
    
    // 디렉터의 매복 명령이 활성화되어 있는지
    const FName HasAmbushOrder(
       TEXT("HasAmbushOrder")
    );

    // 시야가 끊겨도 유지할 매복 목표
    const FName AmbushTargetActor(
       TEXT("AmbushTargetActor")
    );

    // EQS가 선택할 은폐 위치
    const FName AmbushLocation(
       TEXT("AmbushLocation")
    );

    // 몬스터가 은폐 위치에 도착해 대기 중인지
    const FName IsAmbushReady(
       TEXT("IsAmbushReady")
    );

    // 목표가 가까워져 기습을 시작해야 하는지
    const FName ShouldSpringAmbush(
       TEXT("ShouldSpringAmbush")
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
    
    // DataAsset에서 적용된 이동 도착 허용 반경을
    // Behavior Tree가 사용할 블랙보드에 저장
    UBlackboardComponent* BlackboardComponent =
    GetBlackboardComponent();

    // BT 실행 요청 후에도 Blackboard가 없다면 실제 초기화 문제
    if (!IsValid(BlackboardComponent))
    {
       BARU_NET_LOG(
          this,
          LogBaruAI,
          Error,
          TEXT(
             "Monster Blackboard is missing after "
             "RunBehaviorTree. Check the BehaviorTree "
             "and Blackboard assets."
          )
       );

       return;
    }

    // Blackboard가 준비된 뒤 이동 도착 허용 반경 저장
    BlackboardComponent->SetValueAsFloat(
       BaruMonsterBlackboardKeys::MoveAcceptanceRadius,
       MonsterNavigationComponent->GetMoveAcceptanceRadius()
    );
    
    // BT 실행에 성공한 뒤 초기 감지 상태를 반영
    UpdateBlackboardFromPerceptionState();

    // 초기화 전에 접수된 디렉터 명령도 Blackboard에 반영
    UpdateBlackboardFromDirectorState();

    // BT와 Blackboard 초기화가 끝난 뒤 위협도 갱신 시작
    StartThreatUpdates();

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
    
    // 근접 전투 중 순간적인 시야 손실을 보정할 거리
    CombatTargetRetentionDistance =
       FMath::Max(
          0.0f,
          MonsterDataAsset.CombatTargetRetentionDistance
       );

    // 거리와 벽 상태를 다시 검사하는 간격
    CombatTargetRetentionCheckInterval =
       FMath::Max(
          0.05f,
          MonsterDataAsset.CombatTargetRetentionCheckInterval
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
    
    SelectHighestThreatVisiblePlayer();
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
    
    SelectHighestThreatVisiblePlayer();
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
    // 조종 중인 몬스터가 있는 서버에서만 감지 결과 처리
    if (!HasAuthority() || !IsValid(GetPawn()))
    {
       return;
    }

    // 사망한 몬스터는 새로운 추적 대상을 선택하지 않음
    if (GetPawn()->Implements<UCombatInterface>() &&
       ICombatInterface::Execute_IsDead(GetPawn()))
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
       // 이전 프레임에 같은 플레이어의 시야 손실을 보류하고 있었다면
       // 다시 발견된 것이므로 손실 검사를 취소
       CancelCombatTargetRetention(SensedPawn);
       
       // 현재 보이는 플레이어가 있으므로
       // 이전 수색에 사용하던 마지막 목격 기억을 제거
       ClearLastKnownTargetLocation();

       // 발견한 플레이어를 추적 후보 목록에 추가
       // 현재 보이는 플레이어 중 위협도 점수가 가장 높은 대상 선택
       AddVisiblePlayerCandidate(SensedPawn);

       /*
        * 매복 중 플레이어를 직접 발견했다면
        * 먼저 선택된 추적 대상을 Blackboard에 반영
        *
        * TargetActor가 먼저 설정되어야 매복 명령이 해제될 때
        * Behavior Tree가 대기 상태를 거치지 않고
        * 바로 Chase Player 분기로 전환할 수 있음
        */
       UpdateBlackboardFromPerceptionState();

       /*
       * 플레이어를 직접 발견했다면 현재 추적 행동으로 전환
       * Investigate:
       * 지정 위치를 확인하라는 명령의 목적을 달성했으므로 해제
       * Ambush:
       * 숨어서 기다리는 단계가 끝났으므로 해제
       * Hold:
       * 전투가 끝난 뒤 원래 위치로 복귀해야 하므로 유지
       */
       if (IsValid(CurrentTarget.Get()))
       {
          switch (DirectorCommand)
          {
          case EBaruMonsterDirectorCommand::Investigate:
             BARU_NET_LOG(
                this,
                LogBaruAI,
                Log,
                TEXT(
                   "Director investigation completed by detection: "
                   "Target=%s"
                ),
                *GetNameSafe(CurrentTarget.Get())
             );

             ClearDirectorCommand();
             break;

          case EBaruMonsterDirectorCommand::Ambush:
             BARU_NET_LOG(
                this,
                LogBaruAI,
                Log,
                TEXT(
                   "Ambush converted to normal chase: "
                   "Detected target=%s"
                ),
                *GetNameSafe(CurrentTarget.Get())
             );

             ClearDirectorCommand();
             break;

          default:
             break;
          }
       }

       BARU_NET_LOG(
          this,
          LogBaruAI,
          Log,
          TEXT(
             "Player detected: %s / "
             "Visible candidates: %d / "
             "Current target: %s"
          ),
          *GetNameSafe(Actor),
          VisiblePlayerCandidates.Num(),
          *GetNameSafe(CurrentTarget.Get())
       );

       return;
    }
    
    // 플레이어를 마지막으로 실제 감지한 위치
    const FVector LostTargetLocation =
       Stimulus.StimulusLocation;

    // 현재 전투 중인 플레이어를 놓친 경우
    const bool bLostCurrentTarget =
       CurrentTarget.Get() == SensedPawn;

    /*
     * 현재 추적 대상이고 가까운 거리이며
     * 플레이어와 몬스터 사이에 건물 벽이 없다면
     * 순간적인 Perception 손실로 판단하고 추적을 유지
     */
    if (bLostCurrentTarget &&
       CanRetainCombatTarget(SensedPawn))
    {
       BeginCombatTargetRetention(
          SensedPawn,
          LostTargetLocation
       );

       BARU_NET_LOG(
          this,
          LogBaruAI,
          Verbose,
          TEXT(
             "Combat target sight loss deferred: "
             "Target=%s / Location=%s"
          ),
          *GetNameSafe(SensedPawn),
          *LostTargetLocation.ToString()
       );

       return;
    }

    // 벽에 가려졌거나 유지 거리 밖이라면
    // 정상적인 시야 손실로 확정
    ConfirmPlayerLost(
       SensedPawn,
       LostTargetLocation
    );
}

bool ABaruMonsterAIController::CanRetainCombatTarget(
    APawn* TargetPawn
) const
{
    if (!IsValid(TargetPawn) ||
        !IsValid(GetPawn()) ||
        CombatTargetRetentionDistance <= 0.0f)
    {
        return false;
    }

    // 죽은 플레이어는 전투 대상으로 유지하지 않음
    if (TargetPawn->Implements<UCombatInterface>() &&
        ICombatInterface::Execute_IsDead(TargetPawn))
    {
        return false;
    }

    const FVector MonsterLocation =
        GetPawn()->GetActorLocation();

    const FVector TargetLocation =
        TargetPawn->GetActorLocation();

    // 지정한 전투 유지 거리 밖이라면 정상적으로 놓침
    if (FVector::DistSquared(
            MonsterLocation,
            TargetLocation
        ) >
        FMath::Square(
            CombatTargetRetentionDistance
        ))
    {
        return false;
    }

    UWorld* World = GetWorld();

    if (!IsValid(World))
    {
        return false;
    }

    FVector MonsterViewLocation;
    FRotator MonsterViewRotation;

    // 몬스터의 눈 위치에서 검사 시작
    GetPawn()->GetActorEyesViewPoint(
        MonsterViewLocation,
        MonsterViewRotation
    );

    // 플레이어 몸통에 가까운 위치를 검사 대상으로 사용
    const FVector TargetViewLocation =
        TargetPawn->GetPawnViewLocation();

    FCollisionObjectQueryParams ObjectQueryParams;

    /*
     * 다른 몬스터나 플레이어가 잠깐 사이를 지나가는 것은 무시하고
     * 건물 벽과 고정된 구조물만 시야 차단물로 취급
     */
    ObjectQueryParams.AddObjectTypesToQuery(
        ECC_WorldStatic
    );

    FCollisionQueryParams QueryParams(
        SCENE_QUERY_STAT(
            BaruCombatTargetRetentionTrace
        ),
        false,
        GetPawn()
    );

    QueryParams.AddIgnoredActor(TargetPawn);

    const bool bBlockedByStaticObject =
        World->LineTraceTestByObjectType(
            MonsterViewLocation,
            TargetViewLocation,
            ObjectQueryParams,
            QueryParams
        );

    return !bBlockedByStaticObject;
}

void ABaruMonsterAIController::
BeginCombatTargetRetention(
    APawn* TargetPawn,
    const FVector& LastVisibleLocation
)
{
    if (!IsValid(TargetPawn))
    {
        return;
    }

    // 이전 대상에 대한 보류 검사가 남아 있다면 먼저 정리
    CancelCombatTargetRetention();

    PendingLostCombatTarget = TargetPawn;
    PendingLostCombatTargetLocation =
        LastVisibleLocation;

    /*
     * 반복 타이머 대신 한 번만 예약
     * 매 검사에서 조건이 유지될 때 다음 검사를 다시 예약
     */
    GetWorldTimerManager().SetTimer(
        CombatTargetRetentionTimerHandle,
        this,
        &ABaruMonsterAIController::
            ReevaluateCombatTargetRetention,
        CombatTargetRetentionCheckInterval,
        false
    );
}

void ABaruMonsterAIController::
ReevaluateCombatTargetRetention()
{
    APawn* TargetPawn =
        PendingLostCombatTarget.Get();

    // 대상이 제거된 경우 저장된 마지막 위치를 이용해 손실 확정
    if (!IsValid(TargetPawn))
    {
        const FVector LastVisibleLocation =
            PendingLostCombatTargetLocation;

        CancelCombatTargetRetention();

        RemoveInvalidPlayerCandidates();
        SelectHighestThreatVisiblePlayer();

        if (!IsValid(CurrentTarget.Get()))
        {
            RememberLastKnownTargetLocation(
                LastVisibleLocation
            );
        }

        UpdateBlackboardFromPerceptionState();
        return;
    }

    /*
     * 가까운 거리를 유지하고 건물 벽도 없다면
     * 플레이어가 옆이나 뒤에 있더라도 계속 전투 대상으로 유지
     */
    if (CanRetainCombatTarget(TargetPawn))
    {
        // 현재까지 벽 없이 추적한 위치를 최신 상태로 갱신
        PendingLostCombatTargetLocation =
            TargetPawn->GetActorLocation();

        GetWorldTimerManager().SetTimer(
            CombatTargetRetentionTimerHandle,
            this,
            &ABaruMonsterAIController::
                ReevaluateCombatTargetRetention,
            CombatTargetRetentionCheckInterval,
            false
        );

        return;
    }

    // 벽이 생겼거나 플레이어가 멀어진 순간 손실 확정
    const FVector LastVisibleLocation =
        PendingLostCombatTargetLocation;

    ConfirmPlayerLost(
        TargetPawn,
        LastVisibleLocation
    );
}

void ABaruMonsterAIController::ConfirmPlayerLost(
    APawn* LostPlayer,
    const FVector& LastVisibleLocation
)
{
    if (!IsValid(LostPlayer))
    {
        return;
    }

    // 후보 목록에서 제거하기 전에 현재 대상이었는지 기록
    const bool bLostCurrentTarget =
        CurrentTarget.Get() == LostPlayer;

    // 해당 플레이어의 보류 타이머가 있다면 함께 정리
    CancelCombatTargetRetention(LostPlayer);

    // 실제 시야 후보 목록에서 제거
    RemoveVisiblePlayerCandidate(LostPlayer);

    /*
     * 현재 대상을 놓쳤고 대신 추적할 다른 플레이어도 없다면
     * 마지막으로 벽 없이 확인했던 위치를 수색
     */
    if (bLostCurrentTarget &&
        !IsValid(GetCurrentTarget()))
    {
        RememberLastKnownTargetLocation(
            LastVisibleLocation
        );
    }

    UpdateBlackboardFromPerceptionState();

    BARU_NET_LOG(
        this,
        LogBaruAI,
        Log,
        TEXT(
            "Player loss confirmed: %s / "
            "Visible candidates: %d / "
            "Current target: %s / "
            "Last visible location: %s / "
            "Memory stored: %s"
        ),
        *GetNameSafe(LostPlayer),
        VisiblePlayerCandidates.Num(),
        *GetNameSafe(CurrentTarget.Get()),
        *LastVisibleLocation.ToString(),
        bHasLastKnownTargetLocation
            ? TEXT("true")
            : TEXT("false")
    );
}

void ABaruMonsterAIController::
CancelCombatTargetRetention(
    APawn* TargetPawn
)
{
    /*
     * 특정 플레이어에 대한 취소 요청인데
     * 현재 보류 중인 대상과 다르다면 건드리지 않음
     */
    if (IsValid(TargetPawn) &&
        PendingLostCombatTarget.Get() != TargetPawn)
    {
        return;
    }

    GetWorldTimerManager().ClearTimer(
        CombatTargetRetentionTimerHandle
    );

    PendingLostCombatTarget.Reset();
    PendingLostCombatTargetLocation =
        FVector::ZeroVector;
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

void ABaruMonsterAIController::SelectHighestThreatVisiblePlayer()
{
    if (!HasAuthority() || !IsValid(GetPawn()))
    {
       CurrentTarget.Reset();
       return;
    }

    // 파괴된 대상, 조종되지 않는 대상, 사망한 대상을 제거
    VisiblePlayerCandidates.RemoveAll(
       [](const TWeakObjectPtr<APawn>& Candidate)
       {
          APawn* CandidatePawn = Candidate.Get();

          if (!IsValid(CandidatePawn) ||
             !CandidatePawn->IsPlayerControlled())
          {
             return true;
          }

          return CandidatePawn->Implements<UCombatInterface>() &&
             ICombatInterface::Execute_IsDead(CandidatePawn);
       }
    );

    APawn* BestTarget = nullptr;
    float BestScore = -1.0f;

    for (const TWeakObjectPtr<APawn>& Candidate :
       VisiblePlayerCandidates)
    {
       APawn* CandidatePawn = Candidate.Get();
       const float Score = CalculateThreatScore(CandidatePawn);

       // 동점이라면 현재 대상을 우선해서 불필요한 전환 방지
       if (Score > BestScore ||
          (Score == BestScore &&
           CandidatePawn == CurrentTarget.Get()))
       {
          BestScore = Score;
          BestTarget = CandidatePawn;
       }
    }

    if (CurrentTarget.Get() == BestTarget)
    {
       return;
    }

    CurrentTarget = BestTarget;

    BARU_NET_LOG(
       this,
       LogBaruAI,
       Log,
       TEXT("Threat target changed: %s / Score=%.1f"),
       *GetNameSafe(BestTarget),
       BestScore
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

    /*
    * SightMemoryDuration은 이제 위치를 잃은 순간부터가 아니라
    * 마지막 목격 위치에 도착한 뒤 주변을 수색하는 시간으로 사용
    * 수색 종료는 Behavior Tree의 전용 수색 태스크가
    * CompleteLastKnownTargetSearch를 호출하여 처리
    */
    
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

void ABaruMonsterAIController::CompleteLastKnownTargetSearch()
{
    // 마지막 목격 위치 판단과 Blackboard 변경은
    // 서버에서만 처리
    if (!HasAuthority())
    {
       return;
    }

    BARU_NET_LOG(
       this,
       LogBaruAI,
       Log,
       TEXT(
          "Last known target area search completed: %s"
       ),
       *LastKnownTargetLocation.ToString()
    );

    // 위치, 유효 여부, 타이머 및 Blackboard 값을 한 번에 정리
    ClearLastKnownTargetLocation();
}

void ABaruMonsterAIController::UpdateBlackboardFromPerceptionState()
{
    UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();

    if (!IsValid(BlackboardComponent))
    {
       /*
        * 스폰 직후에는 감지 이벤트가 BT 초기화보다 먼저 올 수 있음.
        * 감지 정보는 멤버 변수에 유지하고 Blackboard 반영만 보류한다.
        * 초기화가 끝나면 이 함수를 다시 호출해 반영한다.
        */
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

//----------------
// 위협도 / 어그로
//----------------

float ABaruMonsterAIController::CalculateThreatScore(APawn* CandidatePawn) const
{
    const ABaruMonsterCharacter* MonsterCharacter =
        Cast<ABaruMonsterCharacter>(GetPawn());

    if (!IsValid(MonsterCharacter) || !IsValid(CandidatePawn))
    {
        return 0.0f;
    }

    const UBaruMonsterDataAsset* MonsterData = MonsterCharacter->GetMonsterDataAsset();

    if (!IsValid(MonsterData))
    {
        return 0.0f;
    }

    const float Distance = FVector::Distance(
        MonsterCharacter->GetActorLocation(),
        CandidatePawn->GetActorLocation()
    );

    const float ReferenceDistance =
        FMath::Max(1.0f, MonsterData->ThreatDistanceReference);

    // 가까울수록 높고, 기준 거리 이상이면 0점
    const float DistanceScore =
        FMath::Clamp(
            1.0f - Distance / ReferenceDistance,
            0.0f,
            1.0f
        ) * FMath::Max(0.0f, MonsterData->ThreatDistanceWeight);

    const float* StoredThreat = DamageThreatByPlayer.Find(
        TWeakObjectPtr<APawn>(CandidatePawn)
    );

    const float DamageScore = StoredThreat
        ? FMath::Max(0.0f, *StoredThreat)
        : 0.0f;

    // 현재 대상에게 유지 보너스 부여
    const float RetentionScore =
        CandidatePawn == CurrentTarget.Get()
        ? FMath::Max(0.0f, MonsterData->CurrentTargetThreatBonus)
        : 0.0f;

    return DistanceScore + DamageScore + RetentionScore;
}

void ABaruMonsterAIController::StartThreatUpdates()
{
    // 초기화가 다시 호출되어도 타이머가 중복되지 않도록 정리
    GetWorldTimerManager().ClearTimer(ThreatUpdateTimerHandle);

    if (!HasAuthority())
    {
        return;
    }

    ABaruMonsterCharacter* MonsterCharacter =
        Cast<ABaruMonsterCharacter>(GetPawn());

    if (!IsValid(MonsterCharacter) ||
        ICombatInterface::Execute_IsDead(MonsterCharacter))
    {
        return;
    }

    const UBaruMonsterDataAsset* MonsterData =
        MonsterCharacter->GetMonsterDataAsset();

    if (!IsValid(MonsterData))
    {
        return;
    }

    LastThreatUpdateTime = GetWorld()->GetTimeSeconds();

    GetWorldTimerManager().SetTimer(
        ThreatUpdateTimerHandle,
        this,
        &ABaruMonsterAIController::UpdateThreat,
        FMath::Max(0.1f, MonsterData->ThreatUpdateInterval),
        true
    );
}

void ABaruMonsterAIController::UpdateThreat()
{
    if (!HasAuthority())
    {
        return;
    }

    ABaruMonsterCharacter* MonsterCharacter =
        Cast<ABaruMonsterCharacter>(GetPawn());

    // 시체가 남더라도 위협도 갱신은 중단
    if (!IsValid(MonsterCharacter) ||
        ICombatInterface::Execute_IsDead(MonsterCharacter))
    {
        GetWorldTimerManager().ClearTimer(ThreatUpdateTimerHandle);
        DamageThreatByPlayer.Reset();
        return;
    }

    const UBaruMonsterDataAsset* MonsterData =
        MonsterCharacter->GetMonsterDataAsset();

    if (!IsValid(MonsterData))
    {
        return;
    }

    const double CurrentTime = GetWorld()->GetTimeSeconds();
    const float ElapsedSeconds = static_cast<float>(
        FMath::Max(0.0, CurrentTime - LastThreatUpdateTime)
    );

    LastThreatUpdateTime = CurrentTime;

    const float DecayAmount =
        FMath::Max(0.0f, MonsterData->ThreatDecayPerSecond) *
        ElapsedSeconds;

    for (auto It = DamageThreatByPlayer.CreateIterator(); It; ++It)
    {
        APawn* PlayerPawn = It.Key().Get();

        // 사라졌거나 사망한 플레이어의 기록 정리
        if (!IsValid(PlayerPawn) ||
            !PlayerPawn->IsPlayerControlled() ||
            (PlayerPawn->Implements<UCombatInterface>() &&
             ICombatInterface::Execute_IsDead(PlayerPawn)))
        {
            It.RemoveCurrent();
            continue;
        }

        It.Value() = FMath::Max(0.0f, It.Value() - DecayAmount);

        if (It.Value() <= 0.0f)
        {
            It.RemoveCurrent();
        }
    }

    // 그로기 중에도 점수는 감소하지만 주기적 대상 전환은 보류
    UAbilitySystemComponent* MonsterASC =
        MonsterCharacter->GetAbilitySystemComponent();

    if (IsValid(MonsterASC) &&
        MonsterASC->HasMatchingGameplayTag(
            FBaruGameplayTags::Get().State_Debuff_Groggy
        ))
    {
        return;
    }

    SelectHighestThreatVisiblePlayer();

    // 현재 보이는 대상이 있으면 이전 수색 기억은 사용하지 않음
    if (IsValid(CurrentTarget.Get()))
    {
        ClearLastKnownTargetLocation();
    }

    UpdateBlackboardFromPerceptionState();
}

void ABaruMonsterAIController::RegisterDamageThreat(
    APawn* AttackerPawn,
    float DamageAmount
)
{
    if (!HasAuthority() ||
        !IsValid(AttackerPawn) ||
        !AttackerPawn->IsPlayerControlled() ||
        !FMath::IsFinite(DamageAmount) ||
        DamageAmount <= 0.0f)
    {
        return;
    }

    if (AttackerPawn->Implements<UCombatInterface>() &&
        ICombatInterface::Execute_IsDead(AttackerPawn))
    {
        return;
    }

    ABaruMonsterCharacter* MonsterCharacter =
        Cast<ABaruMonsterCharacter>(GetPawn());

    if (!IsValid(MonsterCharacter) ||
        ICombatInterface::Execute_IsDead(MonsterCharacter))
    {
        return;
    }
    
    /*
    * 매복 중 피해를 받았다면 이미 위치가 노출된 것으로 판단
    * 현재 매복 명령을 해제하고 기존 피격 대응 로직으로 전환
    *
    * 아래의 기존 코드가 공격자의 위치를 마지막 목격 위치로
    * 저장하고 피해 위협도도 정상적으로 누적함
    */
    if (DirectorCommand ==
       EBaruMonsterDirectorCommand::Ambush)
    {
       BARU_NET_LOG(
          this,
          LogBaruAI,
          Log,
          TEXT(
             "Ambush cancelled by damage: "
             "Attacker=%s / Damage=%.1f"
          ),
          *GetNameSafe(AttackerPawn),
          DamageAmount
       );

       ClearDirectorCommand();
    }

    const UBaruMonsterDataAsset* MonsterData =
        MonsterCharacter->GetMonsterDataAsset();

    if (!IsValid(MonsterData))
    {
        return;
    }

    // 새 피해를 더하기 전에 기존 위협도에 경과시간만큼 감소 적용
    UpdateThreat();
    
    // 공격자가 시야에 보이지 않더라도,
    // 공격받은 몬스터는 공격이 날아온 위치를 조사하도록 처리
    if (!IsValid(CurrentTarget.Get()))
    {
       // 공격자의 현재 위치를 마지막 확인 위치로 기억
       RememberLastKnownTargetLocation(
          AttackerPawn->GetActorLocation()
       );

       // Behavior Tree가 즉시 조사 분기로 넘어갈 수 있도록
       // 변경된 위치 정보를 Blackboard에 반영
       UpdateBlackboardFromPerceptionState();

       BARU_NET_LOG(
          this,
          LogBaruAI,
          Log,
          TEXT("Hit source location remembered: %s / Attacker=%s"),
          *AttackerPawn->GetActorLocation().ToString(),
          *GetNameSafe(AttackerPawn)
       );
    }

    float& StoredThreat = DamageThreatByPlayer.FindOrAdd(
        TWeakObjectPtr<APawn>(AttackerPawn)
    );

    StoredThreat = FMath::Clamp(
        StoredThreat +
            DamageAmount * FMath::Max(0.0f, MonsterData->ThreatPerDamage),
        0.0f,
        FMath::Max(0.0f, MonsterData->MaxDamageThreat)
    );

    BARU_NET_LOG(
        this,
        LogBaruAI,
        Log,
        TEXT("Damage threat added: %s / Threat=%.1f"),
        *GetNameSafe(AttackerPawn),
        StoredThreat
    );

    // 피해 반영 후 재평가
    // 공격자를 시야 후보에 강제로 추가하지는 않음
    UpdateThreat();
}

void ABaruMonsterAIController::OnUnPossess()
{
    GetWorldTimerManager().ClearTimer(ThreatUpdateTimerHandle);
    GetWorldTimerManager().ClearTimer(SightMemoryTimerHandle);

    DamageThreatByPlayer.Reset();
    VisiblePlayerCandidates.Reset();
    CurrentTarget.Reset();
    AmbushTarget.Reset();
    
    // 보류 중인 전투 대상 유지 검사 정리
    CancelCombatTargetRetention();

    LastKnownTargetLocation = FVector::ZeroVector;
    bHasLastKnownTargetLocation = false;
    LastThreatUpdateTime = 0.0;
    
    // 다음 몬스터에게 이전 디렉터 명령이 전달되지 않도록 정리
    DirectorCommand = EBaruMonsterDirectorCommand::None;
    DirectorTargetLocation = FVector::ZeroVector;

    UpdateBlackboardFromDirectorState();

    // 기존 Blackboard의 추적·수색 정보도 제거
    if (GetBlackboardComponent())
    {
       UpdateBlackboardFromPerceptionState();
    }
    
    ReleaseActiveTacticalRoute();

    Super::OnUnPossess();

    // 다음 조종 대상에게 이전 감지 정보가 남지 않도록 정리
    if (IsValid(MonsterPerceptionComponent))
    {
       MonsterPerceptionComponent->ForgetAll();
    }
}

void ABaruMonsterAIController::EndPlay(
    const EEndPlayReason::Type EndPlayReason
)
{
    ReleaseActiveTacticalRoute();
    
    GetWorldTimerManager().ClearTimer(ThreatUpdateTimerHandle);
    GetWorldTimerManager().ClearTimer(SightMemoryTimerHandle);

    if (IsValid(MonsterPerceptionComponent))
    {
       MonsterPerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(
          this,
          &ABaruMonsterAIController::HandleTargetPerceptionUpdated
       );
    }

    DamageThreatByPlayer.Reset();
    VisiblePlayerCandidates.Reset();
    CurrentTarget.Reset();
    
    // Controller가 제거된 뒤 전투 대상 검사 콜백이 실행되지 않도록 정리
    CancelCombatTargetRetention();

    Super::EndPlay(EndPlayReason);
}

//----------------
// 디렉터 명령
//----------------

bool ABaruMonsterAIController::ReceiveDirectorAmbushCommand(
    APawn* TargetPlayer
)
{
    // 매복 명령은 서버에서만 처리
    if (!HasAuthority() ||
       !IsValid(TargetPlayer) ||
       !TargetPlayer->IsPlayerControlled())
    {
       return false;
    }

    // 사망한 플레이어는 매복 대상으로 사용하지 않음
    if (TargetPlayer->Implements<UCombatInterface>() &&
       ICombatInterface::Execute_IsDead(TargetPlayer))
    {
       return false;
    }

    ABaruMonsterCharacter* MonsterCharacter =
       Cast<ABaruMonsterCharacter>(GetPawn());

    // 조종 중인 몬스터가 없거나 사망했다면 명령 거부
    if (!IsValid(MonsterCharacter) ||
       ICombatInterface::Execute_IsDead(MonsterCharacter))
    {
       return false;
    }

    const UBaruMonsterDataAsset* MonsterData =
       MonsterCharacter->GetMonsterDataAsset();

    // DataAsset에서 매복이 허용된 몬스터만 명령 접수
    if (!IsValid(MonsterData) ||
       !MonsterData->bCanAmbush)
    {
       return false;
    }
    
    // 기존 포위 경로의 예약과 관련 상태 정리
    ReleaseActiveTacticalRoute();

    // 이전 명령과 새로운 매복 명령을 구분
    ++DirectorCommandRevision;

    DirectorCommand =
       EBaruMonsterDirectorCommand::Ambush;

    AmbushTarget = TargetPlayer;
    DirectorTargetLocation = FVector::ZeroVector;

    // 기존 조사 명령을 해제하고
    // 매복 목표와 초기 상태를 Blackboard에 반영
    UpdateBlackboardFromDirectorState();

    BARU_NET_LOG(
       this,
       LogBaruAI,
       Log,
       TEXT("Director ambush command received: Target=%s"),
       *GetNameSafe(TargetPlayer)
    );

    return true;
}

bool ABaruMonsterAIController::ReceiveDirectorInvestigateCommand(
    const FVector& TargetLocation
)
{
    // 명령 접수는 서버에서만 처리
    if (!HasAuthority() || TargetLocation.ContainsNaN())
    {
        return false;
    }

    ABaruMonsterCharacter* MonsterCharacter =
        Cast<ABaruMonsterCharacter>(GetPawn());

    // 조종 대상이 없거나 사망했다면 명령 거부
    if (!IsValid(MonsterCharacter) ||
        ICombatInterface::Execute_IsDead(MonsterCharacter))
    {
        return false;
    }
    
    // 기존 포위 경로의 예약과 관련 상태 정리
    ReleaseActiveTacticalRoute();
    
    // 이전 명령의 완료와 새 명령을 구분
    ++DirectorCommandRevision;

    // 새로운 명령으로 기존 명령을 교체
    DirectorCommand = EBaruMonsterDirectorCommand::Investigate;
    DirectorTargetLocation = TargetLocation;
    
    // 새 조사 명령이 매복 명령을 교체하므로 이전 목표 제거
    AmbushTarget.Reset();

    // 추적·수색을 강제로 중단하지 않고 명령 상태만 전달
    // 실제 실행 우선순위는 Behavior Tree에서 결정
    UpdateBlackboardFromDirectorState();

    BARU_NET_LOG(
        this,
        LogBaruAI,
        Log,
        TEXT("Director investigation received: %s"),
        *TargetLocation.ToString()
    );

    // 명령 접수 성공이며 경로·도착 성공을 의미하지 않음
    return true;
}

void ABaruMonsterAIController::ReceiveDirectorHoldCommand()
{
    if (!HasAuthority())
    {
        return;
    }

    ABaruMonsterCharacter* MonsterCharacter =
        Cast<ABaruMonsterCharacter>(GetPawn());

    if (!IsValid(MonsterCharacter) ||
        ICombatInterface::Execute_IsDead(MonsterCharacter))
    {
        return;
    }
    
    // 기존 포위 경로의 예약과 관련 상태 정리
    ReleaseActiveTacticalRoute();
    
    // 이전 명령의 완료와 새 명령을 구분
    ++DirectorCommandRevision;

    DirectorCommand = EBaruMonsterDirectorCommand::Hold;
    DirectorTargetLocation = FVector::ZeroVector;
    
    AmbushTarget.Reset();

    // 조사 분기를 해제해서 대기 분기로 넘어가도록 요청
    UpdateBlackboardFromDirectorState();

    // 추적·수색 중이면 해당 이동은 유지
    if (!IsValid(CurrentTarget.Get()) &&
        !bHasLastKnownTargetLocation)
    {
        StopMovement();
    }

    BARU_NET_LOG(
        this,
        LogBaruAI,
        Log,
        TEXT("Director hold command received.")
    );
}

void ABaruMonsterAIController::ClearDirectorCommand()
{
    if (!HasAuthority())
    {
        return;
    }

	// 조사와 집결 이동 모두 취소 대상으로 처리
	const bool bWasInvestigating =
		DirectorCommand ==
			EBaruMonsterDirectorCommand::Investigate ||
		DirectorCommand ==
			EBaruMonsterDirectorCommand::ExtractionRally;
    
    // 기존 포위 경로의 예약과 관련 상태 정리
    ReleaseActiveTacticalRoute();
    
    // 이전 명령의 완료와 새 명령을 구분
    ++DirectorCommandRevision;

    DirectorCommand = EBaruMonsterDirectorCommand::None;
    DirectorTargetLocation = FVector::ZeroVector;
    
    AmbushTarget.Reset();

    UpdateBlackboardFromDirectorState();

    // 디렉터 조사 이동만 취소하고 추적·수색 이동은 유지
    if (bWasInvestigating &&
        !IsValid(CurrentTarget.Get()) &&
        !bHasLastKnownTargetLocation)
    {
        StopMovement();
    }

    BARU_NET_LOG(
        this,
        LogBaruAI,
        Log,
        TEXT("Director command cleared.")
    );
}

void ABaruMonsterAIController::UpdateBlackboardFromDirectorState()
{
    if (!HasAuthority())
    {
        return;
    }

    UBlackboardComponent* MonsterBlackboard =
        GetBlackboardComponent();

    // BT 초기화 전이면 명령은 멤버 변수에 보관하고,
    // 초기화가 끝난 뒤 이 함수를 다시 호출하여 반영
    if (!IsValid(MonsterBlackboard))
    {
        return;
    }
	
	const bool bHasRally =
		DirectorCommand ==
			EBaruMonsterDirectorCommand::ExtractionRally &&
		ExtractionRallyTarget.IsValid();

	if (bHasRally)
	{
		// 대상을 먼저 기록하고 집결 분기를 활성화
		MonsterBlackboard->SetValueAsObject(
			TEXT("ExtractionRallyTarget"),
			ExtractionRallyTarget.Get()
		);

		MonsterBlackboard->SetValueAsBool(
			TEXT("HasExtractionRallyOrder"),
			true
		);
	}
	else
	{
		MonsterBlackboard->SetValueAsBool(
			TEXT("HasExtractionRallyOrder"),
			false
		);

		MonsterBlackboard->ClearValue(
			TEXT("ExtractionRallyTarget")
		);
	}

    const bool bHasInvestigation =
        DirectorCommand ==
            EBaruMonsterDirectorCommand::Investigate;

    const bool bHasAmbush =
        DirectorCommand ==
            EBaruMonsterDirectorCommand::Ambush &&
        AmbushTarget.IsValid();

    // ---------------------------------------------------------
    // 조사 명령
    // ---------------------------------------------------------

    if (bHasInvestigation)
    {
        // 목적지를 먼저 기록한 뒤 조사 조건 활성화
        MonsterBlackboard->SetValueAsVector(
            BaruMonsterBlackboardKeys::DirectorTargetLocation,
            DirectorTargetLocation
        );

        MonsterBlackboard->SetValueAsBool(
            BaruMonsterBlackboardKeys::HasDirectorInvestigation,
            true
        );
    }
    else
    {
        // 조사 조건을 끈 뒤 기존 목적지 제거
        MonsterBlackboard->SetValueAsBool(
            BaruMonsterBlackboardKeys::HasDirectorInvestigation,
            false
        );

        MonsterBlackboard->ClearValue(
            BaruMonsterBlackboardKeys::DirectorTargetLocation
        );
    }

    // ---------------------------------------------------------
    // 매복 명령
    // ---------------------------------------------------------

    if (bHasAmbush)
    {
        // 목표를 먼저 기록한 뒤 매복 분기 활성화
        MonsterBlackboard->SetValueAsObject(
            BaruMonsterBlackboardKeys::AmbushTargetActor,
            AmbushTarget.Get()
        );

        MonsterBlackboard->SetValueAsBool(
            BaruMonsterBlackboardKeys::HasAmbushOrder,
            true
        );

        // 새로운 매복 명령은 아직 위치를 찾거나
        // 기습 준비를 완료하지 않은 상태로 시작
        MonsterBlackboard->ClearValue(
            BaruMonsterBlackboardKeys::AmbushLocation
        );

        MonsterBlackboard->SetValueAsBool(
            BaruMonsterBlackboardKeys::IsAmbushReady,
            false
        );

        MonsterBlackboard->SetValueAsBool(
            BaruMonsterBlackboardKeys::ShouldSpringAmbush,
            false
        );
    }
    else
    {
        // 매복 분기를 먼저 비활성화
        MonsterBlackboard->SetValueAsBool(
            BaruMonsterBlackboardKeys::HasAmbushOrder,
            false
        );

        // 이전 매복에서 사용한 모든 값 제거
        MonsterBlackboard->ClearValue(
            BaruMonsterBlackboardKeys::AmbushTargetActor
        );

        MonsterBlackboard->ClearValue(
            BaruMonsterBlackboardKeys::AmbushLocation
        );

        MonsterBlackboard->SetValueAsBool(
            BaruMonsterBlackboardKeys::IsAmbushReady,
            false
        );

        MonsterBlackboard->SetValueAsBool(
            BaruMonsterBlackboardKeys::ShouldSpringAmbush,
            false
        );
    }
    
    {
        UBlackboardComponent* EncirclementBlackboard =
          GetBlackboardComponent();

        if (IsValid(EncirclementBlackboard))
        {
           const bool bHasEncirclementOrder =
             DirectorCommand ==
                EBaruMonsterDirectorCommand::Encircle &&
             EncirclementTarget.IsValid() &&
             (bUsesDynamicEncirclement || ActiveTacticalRoute.IsValid());

           if (bHasEncirclementOrder)
           {
              // 목적지들을 먼저 기록한 뒤 Bool을 켜야
              // Behavior Tree가 빈 위치로 먼저 실행되지 않음
              EncirclementBlackboard->SetValueAsVector(
                TEXT("EncirclementRouteLocation"),
                EncirclementRouteLocation
             );

              EncirclementBlackboard->SetValueAsVector(
                TEXT("EncirclementBlockLocation"),
                EncirclementBlockLocation
             );

              EncirclementBlackboard->SetValueAsBool(
                TEXT("HasEncirclementOrder"),
                true
             );
           }
           else
           {
              EncirclementBlackboard->SetValueAsBool(
                TEXT("HasEncirclementOrder"),
                false
             );

              EncirclementBlackboard->ClearValue(
                TEXT("EncirclementRouteLocation")
             );

              EncirclementBlackboard->ClearValue(
                TEXT("EncirclementBlockLocation")
             );
           }
        }
    }
    
}

bool ABaruMonsterAIController::ReceiveDirectorEncirclementCommand(
    APawn* TargetPlayer,
    ABaruMonsterTacticalRoute* TacticalRoute
)
{
    if (!HasAuthority() ||
        !IsValid(TargetPlayer) ||
        !IsValid(TacticalRoute))
    {
        return false;
    }

    ABaruMonsterCharacter* ControlledMonster =
        Cast<ABaruMonsterCharacter>(GetPawn());

    if (!IsValid(ControlledMonster))
    {
        return false;
    }

    // 같은 명령이 반복 전달되면 이동을 처음부터 다시 시작하지 않음
    if (DirectorCommand ==
            EBaruMonsterDirectorCommand::Encircle &&
        EncirclementTarget.Get() == TargetPlayer &&
        ActiveTacticalRoute.Get() == TacticalRoute)
    {
        return true;
    }

    // 다른 몬스터가 사용 중이거나 잘못 배치된 경로면 거절
    if (!TacticalRoute->TryReserve(ControlledMonster))
    {
        return false;
    }

    // 이전에 다른 전술 경로를 사용 중이었다면 예약 해제
    if (ABaruMonsterTacticalRoute* PreviousRoute =
            ActiveTacticalRoute.Get())
    {
        if (PreviousRoute != TacticalRoute)
        {
            PreviousRoute->ReleaseReservation(
                ControlledMonster
            );
        }
    }

    DirectorCommand =
        EBaruMonsterDirectorCommand::Encircle;

    // 다른 종류의 디렉터 명령 데이터 정리
    DirectorTargetLocation = FVector::ZeroVector;
    AmbushTarget.Reset();

    EncirclementTarget = TargetPlayer;
    ActiveTacticalRoute = TacticalRoute;
    bUsesDynamicEncirclement = false;

    EncirclementRouteLocation =
        TacticalRoute->GetRouteEntryLocation();

    EncirclementBlockLocation =
        TacticalRoute->GetBlockLocation();

    ++DirectorCommandRevision;

    // 진행 중이던 직접 추적을 중단하고
    // 블랙보드의 포위 분기로 즉시 전환
    StopMovement();
    UpdateBlackboardFromDirectorState();

    return true;
}


// Director가 계산한 좌표로 기존 포위 BT를 실행
bool ABaruMonsterAIController::ReceiveDirectorDynamicEncirclementCommand(
    APawn* TargetPlayer,
    const FVector& RouteLocation,
    const FVector& BlockLocation
)
{
    if (!HasAuthority() ||
        !IsValid(TargetPlayer) ||
        !TargetPlayer->IsPlayerControlled() ||
        RouteLocation.ContainsNaN() ||
        BlockLocation.ContainsNaN())
    {
        return false;
    }

    ABaruMonsterCharacter* ControlledMonster =
        Cast<ABaruMonsterCharacter>(GetPawn());

    const ABaruPlayerState* TargetPlayerState =
        TargetPlayer->GetPlayerState<ABaruPlayerState>();

    // 사망한 몬스터, 다운되거나 사망한 플레이어는 제외
    if (!IsValid(ControlledMonster) ||
        ICombatInterface::Execute_IsDead(ControlledMonster) ||
        !IsValid(TargetPlayerState) ||
        !TargetPlayerState->IsAlive())
    {
        return false;
    }

    // 우회 지점과 저지 지점을 같은 위치로 전달하지 않음
    if (FVector::DistSquared2D(RouteLocation, BlockLocation) <=
        FMath::Square(1.0f))
    {
        return false;
    }

    // 같은 명령은 유지하고, 진행 중인 다른 포위는 덮어쓰지 않음
    // 재배정하려면 Director가 기존 명령을 먼저 해제해야 함
    if (DirectorCommand == EBaruMonsterDirectorCommand::Encircle)
    {
        return bUsesDynamicEncirclement &&
            EncirclementTarget.Get() == TargetPlayer &&
            EncirclementRouteLocation.Equals(RouteLocation, 1.0f) &&
            EncirclementBlockLocation.Equals(BlockLocation, 1.0f);
    }

    // 이전 스플라인 경로의 예약과 포위 데이터를 정리
    ReleaseActiveTacticalRoute();

    bUsesDynamicEncirclement = true;
    EncirclementTarget = TargetPlayer;
    EncirclementRouteLocation = RouteLocation;
    EncirclementBlockLocation = BlockLocation;

    DirectorTargetLocation = FVector::ZeroVector;
    AmbushTarget.Reset();
    DirectorCommand = EBaruMonsterDirectorCommand::Encircle;
    ++DirectorCommandRevision;

    // 실제 이동은 기존 Encirclement 분기의 Move To가 수행
    StopMovement();
    UpdateBlackboardFromDirectorState();

    BARU_NET_LOG(
        this,
        LogBaruAI,
        Log,
        TEXT("Dynamic encirclement received. Target=%s / Route=%s / Block=%s"),
        *GetNameSafe(TargetPlayer),
        *RouteLocation.ToString(),
        *BlockLocation.ToString()
    );

    return true;
}

void ABaruMonsterAIController::
    CompleteDirectorEncirclementCommand()
{
    if (!HasAuthority() ||
        DirectorCommand !=
            EBaruMonsterDirectorCommand::Encircle)
    {
        return;
    }

    // 포위 이동을 마치고 Hold로 전환. 이후 추적·수색 우선순위는 BT가 결정
    DirectorCommand =
        EBaruMonsterDirectorCommand::Hold;

    ++DirectorCommandRevision;

    StopMovement();
    UpdateBlackboardFromDirectorState();
}

void ABaruMonsterAIController::
    ReleaseActiveTacticalRoute()
{
    if (ABaruMonsterTacticalRoute* TacticalRoute =
            ActiveTacticalRoute.Get())
    {
        TacticalRoute->ReleaseReservation(
            Cast<ABaruMonsterCharacter>(GetPawn())
        );
    }
	
	// 명령 교체 시 이전 집결 대상도 정리
	ExtractionRallyTarget.Reset();

    bUsesDynamicEncirclement = false;
    ActiveTacticalRoute.Reset();
    EncirclementTarget.Reset();

    EncirclementRouteLocation = FVector::ZeroVector;
    EncirclementBlockLocation = FVector::ZeroVector;
}

bool ABaruMonsterAIController::ReceiveDirectorExtractionRallyCommand(
	APawn* TargetPlayer
)
{
	if (!HasAuthority() ||
		!IsValid(TargetPlayer) ||
		!TargetPlayer->IsPlayerControlled() ||
		TargetPlayer->GetWorld() != GetWorld())
	{
		return false;
	}

	ABaruMonsterCharacter* Monster =
		Cast<ABaruMonsterCharacter>(GetPawn());

	const ABaruPlayerState* State =
		TargetPlayer->GetPlayerState<ABaruPlayerState>();

	if (!IsValid(Monster) ||
		ICombatInterface::Execute_IsDead(Monster) ||
		!IsValid(State) ||
		!State->IsAlive())
	{
		return false;
	}

	// 우회 담당의 명령은 집결 명령으로 덮어쓰지 않음
	if (DirectorCommand ==
		EBaruMonsterDirectorCommand::Encircle)
	{
		return false;
	}

	// 같은 명령으로 이동을 반복해서 재시작하지 않음
	if (DirectorCommand ==
			EBaruMonsterDirectorCommand::ExtractionRally &&
		ExtractionRallyTarget.Get() == TargetPlayer)
	{
		return true;
	}

	ReleaseActiveTacticalRoute();
	AmbushTarget.Reset();

	DirectorTargetLocation = FVector::ZeroVector;
	ExtractionRallyTarget = TargetPlayer;

	DirectorCommand =
		EBaruMonsterDirectorCommand::ExtractionRally;

	++DirectorCommandRevision;

	// 실제 이동은 집결 BT 분기에서 처리
	StopMovement();
	UpdateBlackboardFromDirectorState();

	return true;
}
