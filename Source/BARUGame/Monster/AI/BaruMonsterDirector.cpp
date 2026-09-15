


#include "Monster/AI/BaruMonsterDirector.h"

#include "Monster/Characters/BaruMonsterCharacter.h"
#include "Monster/Data/BaruMonsterDataAsset.h"
#include "Monster/AI/BaruMonsterAIController.h"
#include "Monster/AI/BaruMonsterTacticalRoute.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Interfaces/CombatInterface.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Gimmicks/BaruControlRoomSpawner.h"
#include "Player/BaruPlayerState.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "BaruLog.h"



ABaruMonsterDirector::ABaruMonsterDirector()
{
	PrimaryActorTick.bCanEverTick = false;

	// 지휘 판단과 목록은 서버에서만 관리
	// 실제 몬스터의 이동·전투 결과는 몬스터 쪽에서 동기화
	bReplicates = false;
}

void ABaruMonsterDirector::BeginPlay()
{
    Super::BeginPlay();

    // 팀 상태 판단과 StateTree는 서버에서만 운영
    if (GetNetMode() == NM_Client)
    {
        return;
    }

    // Actor Tick을 켜지 않고 1초마다 팀 부담도를 계산
    // 첫 계산도 플레이어 생성 시간을 고려해 1초 후 실행
    GetWorldTimerManager().SetTimer(
        TeamBurdenUpdateTimerHandle,
        this,
        &ABaruMonsterDirector::RecalculateTeamBurden,
        1.0f,
        true,
        1.0f
    );
    
    // 설정값이 너무 작더라도 최소 0.1초 간격은 유지
    const float SafeAssignmentUpdateInterval =
        FMath::Max(0.1f, AssignmentUpdateInterval);

    // Actor Tick을 사용하지 않고 일정 간격마다
    // 플레이어 위협도에 맞춰 몬스터 배정 상태 갱신
    GetWorldTimerManager().SetTimer(
        MonsterAssignmentUpdateTimerHandle,
        this,
        &ABaruMonsterDirector::UpdateMonsterAssignments,
        SafeAssignmentUpdateInterval,
        true,
        SafeAssignmentUpdateInterval
    );
    
}

bool ABaruMonsterDirector::RegisterMonster(
    ABaruMonsterCharacter* Monster
)
{
    // 클라이언트에서는 지휘 목록을 변경하지 않음
    // 존재하지 않거나 이미 사망한 몬스터도 등록하지 않음
    // ||는 앞 조건이 true면 뒤 조건을 검사하지 않으므로
    // 무효한 Monster에 사망 인터페이스를 호출하지 않음
    if (GetNetMode() == NM_Client ||
        !IsValid(Monster) ||
        ICombatInterface::Execute_IsDead(Monster))
    {
        return false;
    }

    // 새 몬스터를 넣기 전에 기존 목록의 사망·파괴된 개체 정리
    RemoveInvalidMonsters();

    // 배열에서 사용하는 약한 참조 타입으로 변환
    const TWeakObjectPtr<ABaruMonsterCharacter> MonsterRef(Monster);

    // 같은 몬스터가 여러 번 등록 요청을 보내도 한 번만 보관
    // 이미 등록된 상태이므로 성공으로 처리
    if (RegisteredMonsters.Contains(MonsterRef))
    {
        return true;
    }

    RegisteredMonsters.Add(MonsterRef);

    // 지휘 목록에 실제로 새 항목이 추가됐을 때만 로그 출력
    BARU_NET_LOG(
        this,
        LogBaruAI,
        Log,
        TEXT("Director registered monster: %s / Count=%d"),
        *GetNameSafe(Monster),
        RegisteredMonsters.Num()
    );

    return true;
}

void ABaruMonsterDirector::UnregisterMonster(
    ABaruMonsterCharacter* Monster
)
{
    // 목록 변경은 서버에서만 수행
    if (GetNetMode() == NM_Client)
    {
        return;
    }

    // 살아 있고 등록된 몬스터라면 이전 지휘 명령부터 해제
    // 추적·수색을 유지할지는 Controller의 기존 규칙에 따름
    // 사망한 몬스터라면 FindCommandController 내부 정리에서
    // 목록에서 제외되고 nullptr가 반환됨
    if (ABaruMonsterAIController* MonsterController =
        FindCommandController(Monster))
    {
        MonsterController->ClearDirectorCommand();
    }

    // 지정한 몬스터와 이미 파괴된 참조를 목록에서 제거
    // [Monster]는 아래 검사 코드에서 사용할 인자를 값으로 캡처
    // 검사 결과가 true인 항목만 배열에서 삭제
    RegisteredMonsters.RemoveAll(
        [Monster](const TWeakObjectPtr<ABaruMonsterCharacter>& Entry)
        {
            return !Entry.IsValid() || Entry.Get() == Monster;
        }
    );
    
    // 이 몬스터가 특정 플레이어에게 배정되어 있었다면
    // 등록 해제와 함께 해당 배정 기록도 제거
    MonsterAssignments.Remove(
    TWeakObjectPtr<ABaruMonsterCharacter>(Monster)
    );
    
}

void ABaruMonsterDirector::RemoveInvalidMonsters()
{
    RegisteredMonsters.RemoveAll(
        [](const TWeakObjectPtr<ABaruMonsterCharacter>& Entry)
        {
            // 약한 참조에서 실제 액터 포인터를 가져옴
            ABaruMonsterCharacter* Monster = Entry.Get();

            // 파괴된 몬스터 또는 사망한 몬스터는 지휘 대상에서 제외
            // 시체 액터 자체는 삭제하지 않음
            return !IsValid(Monster) ||
                ICombatInterface::Execute_IsDead(Monster);
        }
    );
}

ABaruMonsterAIController* ABaruMonsterDirector::FindCommandController(
    ABaruMonsterCharacter* Monster
)
{
    // 클라이언트 요청과 무효한 대상은 처리하지 않음
    if (GetNetMode() == NM_Client || !IsValid(Monster))
    {
        return nullptr;
    }

    // 명령 전달 전에 사망·파괴된 몬스터를 제외
    RemoveInvalidMonsters();

    // 이 디렉터에 등록된 몬스터만 지휘 가능
    // 전달받은 액터가 살아 있어도 미등록 상태라면 명령하지 않음
    if (!RegisteredMonsters.Contains(
        TWeakObjectPtr<ABaruMonsterCharacter>(Monster)))
    {
        return nullptr;
    }

    // 몬스터를 현재 조종하는 Controller 확인
    // 아직 조종되지 않거나 다른 종류의 Controller라면 실패
    ABaruMonsterAIController* MonsterController =
        Cast<ABaruMonsterAIController>(Monster->GetController());

    // 실제 명령 처리는 서버 권한이 있는 Controller만 담당
    if (!IsValid(MonsterController) ||
        !MonsterController->HasAuthority())
    {
        return nullptr;
    }

    return MonsterController;
}

bool ABaruMonsterDirector::RequestInvestigation(
    ABaruMonsterCharacter* Monster,
    const FVector& TargetLocation
)
{
    // 유효하지 않은 좌표는 Controller에 전달하지 않음
    if (TargetLocation.ContainsNaN())
    {
        return false;
    }

    // 서버·등록 여부·생존 상태·Controller를 공통 검사
    ABaruMonsterAIController* MonsterController =
        FindCommandController(Monster);

    if (!MonsterController)
    {
        return false;
    }

    // 기존 명령 수신 함수를 호출
    // Controller가 목적지를 Blackboard에 기록하면
    // BT의 Director Investigation 분기가 이동을 담당
    // 여기서는 MoveTo를 직접 호출하지 않으므로
    // 기존 추적·수색과 별도의 이동 요청이 충돌하지 않음
    return MonsterController->ReceiveDirectorInvestigateCommand(
        TargetLocation
    );
}

bool ABaruMonsterDirector::RequestAmbush(
    ABaruMonsterCharacter* Monster,
    APawn* TargetPlayer
)
{
    // 서버에서만 명령하며 유효한 플레이어만 대상으로 사용
    if (GetNetMode() == NM_Client ||
        !IsValid(TargetPlayer) ||
        !TargetPlayer->IsPlayerControlled())
    {
        return false;
    }

    // 등록 여부, 몬스터 생존 상태와 Controller를 공통 검사
    ABaruMonsterAIController* MonsterController =
        FindCommandController(Monster);

    if (!MonsterController)
    {
        return false;
    }

    // 개별 AI가 자신의 DataAsset에서 매복 가능 여부를
    // 다시 확인한 뒤 명령 접수 성공 여부를 반환
    const bool bAccepted =
        MonsterController->ReceiveDirectorAmbushCommand(
            TargetPlayer
        );

    if (bAccepted)
    {
        BARU_NET_LOG(
            this,
            LogBaruAI,
            Log,
            TEXT(
                "Director assigned ambush: "
                "Monster=%s, Target=%s"
            ),
            *GetNameSafe(Monster),
            *GetNameSafe(TargetPlayer)
        );
    }

    return bAccepted;
}

bool ABaruMonsterDirector::RequestEncirclement(
    ABaruMonsterCharacter* Monster,
    APawn* TargetPlayer,
    ABaruMonsterTacticalRoute* TacticalRoute
)
{
    // 포위 허용 상태와 목표·경로의 유효성을 함께 검사
    if (!IsEncirclementAllowed() ||
        GetNetMode() == NM_Client ||
        !IsValid(TargetPlayer) ||
        !TargetPlayer->IsPlayerControlled() ||
        !IsValid(TacticalRoute) ||
        !TacticalRoute->IsRouteConfigured())
    {
        return false;
    }

    const ABaruPlayerState* TargetPlayerState =
        TargetPlayer->GetPlayerState<ABaruPlayerState>();

    // 다운되거나 사망한 플레이어는 차단하지 않음
    if (!IsValid(TargetPlayerState) ||
        !TargetPlayerState->IsAlive())
    {
        return false;
    }

    ABaruMonsterAIController* MonsterController =
        FindCommandController(Monster);

    if (!IsValid(MonsterController))
    {
        return false;
    }

    const bool bAccepted =
        MonsterController->
            ReceiveDirectorEncirclementCommand(
                TargetPlayer,
                TacticalRoute
            );

    if (bAccepted)
    {
        BARU_NET_LOG(
            this,
            LogBaruAI,
            Log,
            TEXT(
                "Director assigned encirclement: "
                "Monster=%s, Target=%s, Route=%s"
            ),
            *GetNameSafe(Monster),
            *GetNameSafe(TargetPlayer),
            *GetNameSafe(TacticalRoute)
        );
    }

    return bAccepted;
}

bool ABaruMonsterDirector::RequestHold(
    ABaruMonsterCharacter* Monster
)
{
    // 등록된 살아 있는 몬스터의 Controller 확인
    ABaruMonsterAIController* MonsterController =
        FindCommandController(Monster);

    if (!MonsterController)
    {
        return false;
    }

    // 대기 명령을 전달
    // 플레이어 추적·수색이 우선이라는 기존 규칙은 유지
    MonsterController->ReceiveDirectorHoldCommand();

    // 명령 함수를 호출할 수 있었음을 반환
    return true;
}

bool ABaruMonsterDirector::RequestClearCommand(
    ABaruMonsterCharacter* Monster
)
{
    ABaruMonsterAIController* MonsterController =
        FindCommandController(Monster);

    if (!MonsterController)
    {
        return false;
    }

    // 명령만 해제하고 지휘 목록에는 계속 보관
    MonsterController->ClearDirectorCommand();

    return true;
}

void ABaruMonsterDirector::EndPlay(
    const EEndPlayReason::Type EndPlayReason
)
{
    // 종료 정리도 서버에서만 수행
    if (GetNetMode() != NM_Client)
    {
        // Director 종료 후 부담도 계산이 다시 호출되지 않도록 정리
        GetWorldTimerManager().ClearTimer(TeamBurdenUpdateTimerHandle);
        
        // Director 종료 후 자동 몬스터 배정이 다시 실행되지 않도록 정리
        GetWorldTimerManager().ClearTimer(
            MonsterAssignmentUpdateTimerHandle
        );
        
            // 명령 해제로 다른 콜백이 실행될 수 있으므로
            // 순회할 목록을 먼저 복사
            const TArray<TWeakObjectPtr<ABaruMonsterCharacter>> Monsters =
                RegisteredMonsters;

            // 디렉터의 원본 목록은 먼저 비움
            RegisteredMonsters.Reset();
        
            // 디렉터 종료 시 플레이어별 위협도 기록도 비움
            PlayerThreatScores.Reset();
        
            // 몬스터와 플레이어 사이의 자동 배정 기록 제거
            MonsterAssignments.Reset();

            for (const TWeakObjectPtr<ABaruMonsterCharacter>& Entry : Monsters)
            {
                ABaruMonsterCharacter* Monster = Entry.Get();

                // 이미 제거됐거나 사망한 몬스터는 건드리지 않음
                if (!IsValid(Monster) ||
                    ICombatInterface::Execute_IsDead(Monster))
                {
                    continue;
                }

                // 살아 있는 몬스터가 이전 디렉터 명령을 계속
                // 보관하지 않도록 Controller의 명령을 해제
                // FindCommandController를 사용하지 않고 직접 확인
                if (ABaruMonsterAIController* MonsterController =
                    Cast<ABaruMonsterAIController>(Monster->GetController()))
                {
                    MonsterController->ClearDirectorCommand();
                }
            }
    }
    
    Super::EndPlay(EndPlayReason);
}

void ABaruMonsterDirector::SetPlayerThreat(
    APawn* PlayerPawn,
    float NewThreat
)
{
    // 위협도 목록은 서버에서만 변경
    // NaN이나 무한대처럼 계산에 사용할 수 없는 값도 거부
    if (GetNetMode() == NM_Client ||
        !FMath::IsFinite(NewThreat))
    {
        return;
    }

    // 값을 기록하기 전에 오래된 플레이어 참조 정리
    RemoveInvalidPlayerThreats();

    // 실제 플레이어가 조종 중인 Pawn만 기록
    // 몬스터와 조종되지 않은 Pawn은 대상에서 제외
    if (!IsValid(PlayerPawn) ||
        !PlayerPawn->IsPlayerControlled())
    {
        return;
    }

    const TWeakObjectPtr<APawn> PlayerRef(PlayerPawn);

    // 디렉터 위협도는 우선 0~100점으로 사용
    const float SafeThreat =
        FMath::Clamp(NewThreat, 0.0f, 100.0f);

    if (SafeThreat <= 0.0f)
    {
        // 기록이 없는 플레이어도 조회 시 0점으로 처리하므로
        // 0점 항목은 따로 보관하지 않음
        PlayerThreatScores.Remove(PlayerRef);
        return;
    }

    // 처음 보는 플레이어면 새 항목 추가
    // 이미 기록된 플레이어면 기존 점수를 교체
    PlayerThreatScores.FindOrAdd(PlayerRef) = SafeThreat;
}

void ABaruMonsterDirector::AddPlayerThreat(
    APawn* PlayerPawn,
    float ThreatDelta
)
{
    if (GetNetMode() == NM_Client ||
        !IsValid(PlayerPawn) ||
        !PlayerPawn->IsPlayerControlled() ||
        !FMath::IsFinite(ThreatDelta))
    {
        return;
    }

    // 저장된 점수에 이번 변화량을 더함
    // 범위 제한과 실제 저장은 SetPlayerThreat에서 공통 처리
    SetPlayerThreat(
        PlayerPawn,
        GetPlayerThreat(PlayerPawn) + ThreatDelta
    );
}

void ABaruMonsterDirector::ReportPlayerDamageThreat(
    APawn* PlayerPawn,
    float DamageAmount
)
{
    // 피해 기반 디렉터 위협도는 서버에서만 계산
    // 플레이어와 피해량이 유효하지 않으면 처리하지 않음
    if (GetNetMode() == NM_Client ||
        !IsValid(PlayerPawn) ||
        !PlayerPawn->IsPlayerControlled() ||
        !FMath::IsFinite(DamageAmount) ||
        DamageAmount <= 0.0f ||
        !FMath::IsFinite(ThreatGainPerDamage) ||
        ThreatGainPerDamage <= 0.0f)
    {
        return;
    }

    // 실제 피해량을 디렉터 위협도 증가량으로 변환
    const float ThreatGain =
        DamageAmount * ThreatGainPerDamage;

    if (!FMath::IsFinite(ThreatGain) ||
        ThreatGain <= 0.0f)
    {
        return;
    }

    const float PreviousThreat =
        GetPlayerThreat(PlayerPawn);

    // 기존 위협도에 피해 기반 증가량 누적
    // 최종 0~100 제한은 AddPlayerThreat 내부에서 처리
    AddPlayerThreat(
        PlayerPawn,
        ThreatGain
    );

    const float NewThreat =
        GetPlayerThreat(PlayerPawn);

    BARU_NET_LOG(
        this,
        LogBaruAI,
        Log,
        TEXT(
            "Player damage threat reported: %s / "
            "Damage=%.1f, Threat=%.1f -> %.1f"
        ),
        *GetNameSafe(PlayerPawn),
        DamageAmount,
        PreviousThreat,
        NewThreat
    );
}

float ABaruMonsterDirector::GetPlayerThreat(
    APawn* PlayerPawn
) const
{
    if (!IsValid(PlayerPawn) ||
        !PlayerPawn->IsPlayerControlled())
    {
        return 0.0f;
    }

    // Find는 기록이 있으면 그 값의 주소를 반환
    // 기록이 없으면 nullptr를 반환하며 새 항목을 만들지 않음
    const float* FoundThreat =
        PlayerThreatScores.Find(
            TWeakObjectPtr<APawn>(PlayerPawn)
        );

    return FoundThreat ? *FoundThreat : 0.0f;
}

void ABaruMonsterDirector::RemoveInvalidPlayerThreats()
{
    // 순회하면서 삭제할 수 있는 Map 반복자 사용
    for (auto It = PlayerThreatScores.CreateIterator(); It; ++It)
    {
        APawn* PlayerPawn = It.Key().Get();

        // 캐릭터가 제거됐거나 더 이상 플레이어가 조종하지 않으면
        // 해당 캐릭터의 위협도 기록도 제거
        if (!IsValid(PlayerPawn) ||
            !PlayerPawn->IsPlayerControlled())
        {
            It.RemoveCurrent();
        }
    }
}

void ABaruMonsterDirector::DecayPlayerThreats(
    float DeltaSeconds
)
{
    // 위협도 감소는 서버에서만 처리
    // 시간이나 감소 설정값이 유효하지 않으면 처리하지 않음
    if (GetNetMode() == NM_Client ||
        !FMath::IsFinite(DeltaSeconds) ||
        DeltaSeconds <= 0.0f ||
        !FMath::IsFinite(PlayerThreatDecayPerSecond) ||
        PlayerThreatDecayPerSecond <= 0.0f)
    {
        return;
    }

    // 감소 계산 전에 파괴되거나 조종이 해제된
    // 플레이어 위협도 기록을 먼저 제거
    RemoveInvalidPlayerThreats();

    const float ThreatDecay =
        PlayerThreatDecayPerSecond * DeltaSeconds;

    for (auto It = PlayerThreatScores.CreateIterator(); It; ++It)
    {
        const float NewThreat =
            FMath::Max(0.0f, It.Value() - ThreatDecay);

        // 0까지 감소한 위협도는 별도로 보관하지 않음
        if (NewThreat <= 0.0f)
        {
            It.RemoveCurrent();
            continue;
        }

        It.Value() = NewThreat;
    }
}

void ABaruMonsterDirector::RemoveInvalidAssignments()
{
    // 배정 정보는 서버에서만 관리
    if (GetNetMode() == NM_Client)
    {
        return;
    }

    // 반복 도중 항목을 삭제할 수 있는 Map 반복자 사용
    for (auto It = MonsterAssignments.CreateIterator(); It; ++It)
    {
        // 배정 기록에서 실제 몬스터와 플레이어 포인터 가져오기
        ABaruMonsterCharacter* Monster =
            It.Key().Get();

        APawn* PlayerPawn =
            It.Value().Get();

        // 플레이어 Pawn이 유효한 경우 PlayerState 확인
        // PlayerState의 사망·DBNO 상태를 배정 정리에 사용
        const ABaruPlayerState* PlayerState =
            IsValid(PlayerPawn)
                ? PlayerPawn->GetPlayerState<ABaruPlayerState>()
                : nullptr;

        // 다음 몬스터는 더 이상 배정 대상으로 사용할 수 없음:
        // 1. 파괴되거나 유효하지 않음
        // 2. 사망 상태
        // 3. 디렉터의 등록 목록에서 제거됨
        const bool bInvalidMonster =
            !IsValid(Monster) ||
            ICombatInterface::Execute_IsDead(Monster) ||
            !RegisteredMonsters.Contains(
                TWeakObjectPtr<ABaruMonsterCharacter>(Monster)
            );

        // 다음 플레이어는 더 이상 압박 대상으로 사용할 수 없음:
        // 1. Pawn이 파괴되거나 유효하지 않음
        // 2. 플레이어가 조종하지 않는 Pawn
        // 3. PlayerState가 아직 없거나 제거됨
        // 4. DBNO 또는 사망 상태
        const bool bInvalidPlayer =
            !IsValid(PlayerPawn) ||
            !PlayerPawn->IsPlayerControlled() ||
            !IsValid(PlayerState) ||
            !PlayerState->IsAlive();
        
        // 몬스터와 플레이어가 모두 정상이라면
        // 현재 배정을 그대로 유지
        if (!bInvalidMonster && !bInvalidPlayer)
        {
            continue;
        }

        // 몬스터는 살아 있지만 플레이어만 유효하지 않다면
        // 몬스터가 이전 위치로 계속 이동하지 않도록 명령 해제
        if (!bInvalidMonster)
        {
            if (ABaruMonsterAIController* MonsterController =
                FindCommandController(Monster))
            {
                MonsterController->ClearDirectorCommand();
            }
        }
        
        // 유효하지 않은 몬스터·플레이어 배정 기록 제거
        It.RemoveCurrent();
    }
}

int32 ABaruMonsterDirector::GetDesiredMonsterCount(
    APawn* PlayerPawn
) const
{
    // 잘못된 기본 설정이면 몬스터를 배정하지 않음
    if (MaxMonstersPerPlayer <= 0 ||
        !FMath::IsFinite(ThreatPerMonster) ||
        ThreatPerMonster < 1.0f)
    {
        return 0;
    }

    const float CurrentThreat = GetPlayerThreat(PlayerPawn);

    // 위협도가 없는 플레이어에게는
    // 어떤 운영 상태에서도 몬스터를 새로 배정하지 않음
    if (CurrentThreat <= 0.0f)
    {
        return 0;
    }

    // 평상시 기준 배정 수
    int32 DesiredCount =
        FMath::FloorToInt(CurrentThreat / ThreatPerMonster);

    // 상태마다 배정 강도를 조절
    switch (CurrentDirectorState)
    {
    case EBaruDirectorState::Normal:
        // 평상시:
        // 위협도 계산 결과를 그대로 사용
        break;

    case EBaruDirectorState::Pressure:
        // 압박:
        // 평상시보다 한 마리를 추가 배정
        DesiredCount += 1;
        break;

    case EBaruDirectorState::Relief:
        // 완화:
        // 이미 위협도가 있더라도 최대 한 마리만 추가 배정
        DesiredCount = FMath::Min(DesiredCount, 1);
        break;

    case EBaruDirectorState::Extraction:
        // 탈출 저지:
        // 엘리베이터로 복귀하는 플레이어를 압박하기 위해
        // 평상시보다 두 마리를 추가 배정
        DesiredCount += 2;
        break;

    default:
        break;
    }

    // 어떤 상태에서도 플레이어별 최대 배정 수를 넘지 않음
    return FMath::Clamp(
        DesiredCount,
        0,
        MaxMonstersPerPlayer
    );
}

void ABaruMonsterDirector::RecalculateTeamBurden()
{
    // 팀 부담도는 서버에서만 계산
    if (GetNetMode() == NM_Client)
    {
        return;
    }

    UWorld* World = GetWorld();

    if (!IsValid(World))
    {
        return;
    }

    const AGameStateBase* GameState =
        World->GetGameState<AGameStateBase>();

    if (!IsValid(GameState))
    {
        return;
    }

    int32 PlayerCount = 0;
    int32 DBNOPlayerCount = 0;
    int32 DeadPlayerCount = 0;

    float TotalMissingHealthPercent = 0.0f;

    // GameState의 PlayerArray에는 서버가 알고 있는
    // 각 접속 플레이어의 PlayerState가 들어 있음
    for (APlayerState* BasePlayerState : GameState->PlayerArray)
    {
        ABaruPlayerState* BaruPlayerState =
            Cast<ABaruPlayerState>(BasePlayerState);

        if (!IsValid(BaruPlayerState))
        {
            continue;
        }

        const float MaxHealth =
            BaruPlayerState->GetMaxHealth();

        // GAS 초기화 전처럼 최대 체력이 유효하지 않은
        // PlayerState는 이번 계산에서 제외
        if (!FMath::IsFinite(MaxHealth) ||
            MaxHealth <= KINDA_SMALL_NUMBER)
        {
            continue;
        }

        const float CurrentHealth = FMath::Clamp(
            BaruPlayerState->GetHealth(),
            0.0f,
            MaxHealth
        );

        const float HealthRatio =
            CurrentHealth / MaxHealth;

        // 체력 100%면 0, 체력 0이면 100을 더함
        TotalMissingHealthPercent +=
            (1.0f - HealthRatio) * 100.0f;

        PlayerCount++;

        // 사망과 DBNO는 중복으로 세지 않음
        if (BaruPlayerState->IsDead())
        {
            DeadPlayerCount++;
        }
        else if (BaruPlayerState->IsDBNOOnly())
        {
            DBNOPlayerCount++;
        }
    }

    // 계산 가능한 플레이어가 아직 없다면
    // 기존 부담도를 유지하고 다음 타이머를 기다림
    if (PlayerCount <= 0)
    {
        return;
    }

    const float AverageMissingHealth =
        TotalMissingHealthPercent /
        static_cast<float>(PlayerCount);

    const float DBNORatio =
        static_cast<float>(DBNOPlayerCount) /
        static_cast<float>(PlayerCount);

    const float DeadRatio =
        static_cast<float>(DeadPlayerCount) /
        static_cast<float>(PlayerCount);

    // 체력 손실은 그대로 반영
    //
    // 4인 중 1명이 다운 또는 사망하면:
    // 인원 비율 0.25 × 200 = 50점이 추가됨
    //
    // 해당 플레이어의 체력도 0이라면
    // 평균 체력 손실 25점까지 더해져 약 75점이 됨
    const float CalculatedBurden =
        AverageMissingHealth +
        (DBNORatio * 200.0f) +
        (DeadRatio * 200.0f);

    // 최종 0~100 제한과 로그는 기존 함수가 담당
    SetTeamBurden(CalculatedBurden);
}

void ABaruMonsterDirector::SetTeamBurden(
    float NewBurden
)
{
    // 팀 부담도는 서버에서만 관리
    // NaN이나 무한대 값은 상태 판단에 사용할 수 없으므로 거부
    if (GetNetMode() == NM_Client ||
        !FMath::IsFinite(NewBurden))
    {
        return;
    }

    // 팀 부담도는 항상 0~100 범위로 제한
    const float SafeBurden =
        FMath::Clamp(NewBurden, 0.0f, 100.0f);

    // 값이 사실상 같으면 불필요한 기록을 하지 않음
    if (FMath::IsNearlyEqual(TeamBurden, SafeBurden))
    {
        return;
    }

    const float PreviousBurden = TeamBurden;
    TeamBurden = SafeBurden;

    BARU_NET_LOG(
        this,
        LogBaruAI,
        Log,
        TEXT("Team burden changed: %.1f -> %.1f"),
        PreviousBurden,
        TeamBurden
    );
}

void ABaruMonsterDirector::AddTeamBurden(
    float BurdenDelta
)
{
    // 서버에서만 변경하며 잘못된 숫자는 거부
    if (GetNetMode() == NM_Client ||
        !FMath::IsFinite(BurdenDelta))
    {
        return;
    }

    SetTeamBurden(TeamBurden + BurdenDelta);
}

void ABaruMonsterDirector::SetExtractionTarget(
    APawn* TargetPlayer
)
{
    // 탈출 저지 목표는 서버에서만 변경
    if (GetNetMode() == NM_Client)
    {
        return;
    }

    // nullptr가 전달되면 기존 탈출 목표 제거
    if (!IsValid(TargetPlayer))
    {
        ExtractionTargetPlayer.Reset();
        return;
    }

    // 실제 플레이어가 조종하는 Pawn만 목표로 저장
    if (!TargetPlayer->IsPlayerControlled())
    {
        return;
    }

    ExtractionTargetPlayer = TargetPlayer;

    BARU_NET_LOG(
        this,
        LogBaruAI,
        Log,
        TEXT("Extraction target changed: %s"),
        *GetNameSafe(TargetPlayer)
    );
}

void ABaruMonsterDirector::SetExtractionActive(
    bool bNewExtractionActive
)
{
    // 탈출 저지 판단은 서버에서만 변경
    if (GetNetMode() == NM_Client)
    {
        return;
    }

    // 같은 상태를 반복해서 전달받으면 아무것도 하지 않음
    if (bExtractionActive == bNewExtractionActive)
    {
        return;
    }

    bExtractionActive = bNewExtractionActive;
    
    // 탈출 저지가 종료되면 이전 플레이어 목표도 제거
    if (!bExtractionActive)
    {
        ExtractionTargetPlayer.Reset();
    }

    BARU_NET_LOG(
        this,
        LogBaruAI,
        Log,
        TEXT("Extraction active changed: %s"),
        bExtractionActive ? TEXT("true") : TEXT("false")
    );
}

void ABaruMonsterDirector::SetDirectorState(
    EBaruDirectorState NewState
)
{
    // 디렉터의 운영 판단은 서버에서만 처리
    if (GetNetMode() == NM_Client)
    {
        return;
    }

    // 이미 같은 상태라면 중복 처리하지 않음
    if (CurrentDirectorState == NewState)
    {
        return;
    }

    const EBaruDirectorState PreviousState =
        CurrentDirectorState;

    CurrentDirectorState = NewState;

    // 상태 전환이 실제로 발생했을 때만 기록
    // 로그에서 읽기 쉽도록 열거형 값을 이름으로 변환
    const UEnum* StateEnum =
        StaticEnum<EBaruDirectorState>();

    BARU_NET_LOG(
        this,
        LogBaruAI,
        Log,
        TEXT("Director state changed: %s -> %s"),
        *StateEnum->GetNameStringByValue(
            static_cast<int64>(PreviousState)
        ),
        *StateEnum->GetNameStringByValue(
            static_cast<int64>(CurrentDirectorState)
        )
    );
    
    // 상태가 바뀌면 기존 포위를 계속 유지할 수 있는지 즉시 검사
    RefreshEncirclementAssignment();
    
}

ABaruMonsterCharacter*
ABaruMonsterDirector::FindClosestUnassignedMonster(
    const APawn* PlayerPawn
)
{
    // 몬스터 선택은 서버에서만 수행
    // 플레이어 Pawn이 유효하지 않으면 선택하지 않음
    if (GetNetMode() == NM_Client ||
        !IsValid(PlayerPawn))
    {
        return nullptr;
    }

    // 현재까지 발견한 가장 가까운 몬스터
    ABaruMonsterCharacter* ClosestMonster = nullptr;

    // 아직 거리를 비교하지 않았으므로
    // 비교 기준을 가능한 가장 큰 값으로 시작
    double ClosestDistanceSquared =
        TNumericLimits<double>::Max();

    for (const TWeakObjectPtr<ABaruMonsterCharacter>& Entry :
         RegisteredMonsters)
    {
        ABaruMonsterCharacter* Monster = Entry.Get();

        // 파괴됐거나 사망한 몬스터는 선택하지 않음
        if (!IsValid(Monster) ||
            ICombatInterface::Execute_IsDead(Monster))
        {
            continue;
        }

        // 이미 다른 플레이어에게 배정된 몬스터는 제외
        if (MonsterAssignments.Contains(Entry))
        {
            continue;
        }

        // 현재 몬스터를 Baru AIController가 조종하지 않거나
        // 서버 권한이 없는 Controller라면 명령할 수 없으므로 제외
        ABaruMonsterAIController* MonsterController =
            Cast<ABaruMonsterAIController>(
                Monster->GetController()
            );

        if (!IsValid(MonsterController) ||
            !MonsterController->HasAuthority())
        {
            continue;
        }

        // 제곱 거리로 비교하면 매번 제곱근을 계산하지 않아도 됨
        const double DistanceSquared =
            FVector::DistSquared(
                Monster->GetActorLocation(),
                PlayerPawn->GetActorLocation()
            );

        // 지금까지 발견한 몬스터보다 멀다면 건너뜀
        if (DistanceSquared >= ClosestDistanceSquared)
        {
            continue;
        }

        // 더 가까운 미배정 몬스터를 새로운 후보로 저장
        ClosestMonster = Monster;
        ClosestDistanceSquared = DistanceSquared;
    }

    return ClosestMonster;
}

int32 ABaruMonsterDirector::GetAlivePlayerCount() const
{
    UWorld* World = GetWorld();

    if (!IsValid(World))
    {
        return 0;
    }

    AGameStateBase* GameState =
        World->GetGameState();

    if (!IsValid(GameState))
    {
        return 0;
    }

    int32 AlivePlayerCount = 0;

    /*
     * GameState의 PlayerArray에는
     * 서버에 참가한 모든 PlayerState가 들어 있다.
     */
    for (APlayerState* PlayerState :
         GameState->PlayerArray)
    {
        ABaruPlayerState* BaruPlayerState =
            Cast<ABaruPlayerState>(PlayerState);

        if (!IsValid(BaruPlayerState) ||
            !BaruPlayerState->IsAlive())
        {
            continue;
        }

        APawn* PlayerPawn =
            BaruPlayerState->GetPawn();

        /*
         * Pawn이 존재하고 실제 플레이어가 조종하는 경우만 계산한다.
         * 따라서 AI 동료나 아직 스폰되지 않은 관전자는 제외된다.
         */
        if (!IsValid(PlayerPawn) ||
            !PlayerPawn->IsPlayerControlled())
        {
            continue;
        }

        ++AlivePlayerCount;
    }

    return AlivePlayerCount;
}

int32 ABaruMonsterDirector::
GetWaveSizeForCurrentState() const
{
    const int32 AlivePlayerCount =
       GetAlivePlayerCount();

    // 살아 있는 플레이어가 없다면 스폰하지 않음
    if (AlivePlayerCount <= 0)
    {
        return 0;
    }

    int32 MonstersPerPlayer = 0;

    /*
     * 기존 WaveSize 설정값을
     * 이제부터는 '플레이어 1명당 수량'으로 사용한다.
     */
    switch (CurrentDirectorState)
    {
    case EBaruDirectorState::Normal:
        MonstersPerPlayer =
            FMath::Max(0, NormalWaveSize);
        break;

    case EBaruDirectorState::Pressure:
        MonstersPerPlayer =
            FMath::Max(0, PressureWaveSize);
        break;

    case EBaruDirectorState::Relief:
        // 휴식 상태에서는 웨이브를 생성하지 않음
        return 0;

    case EBaruDirectorState::Extraction:
        MonstersPerPlayer =
            FMath::Max(0, ExtractionWaveSize);
        break;

    default:
        return 0;
    }

    return AlivePlayerCount * MonstersPerPlayer;
}

bool ABaruMonsterDirector::TrySpawnDirectorWave(
    APawn* TargetPlayer
)
{
    if (GetNetMode() == NM_Client ||
        !IsValid(TargetPlayer))
    {
        return false;
    }

    // 완화 상태에서는 플레이어가 회복할 수 있도록
    // 새로운 웨이브 생성을 완전히 차단
    if (CurrentDirectorState == EBaruDirectorState::Relief)
    {
        return false;
    }

    UWorld* World = GetWorld();

    if (!IsValid(World))
    {
        return false;
    }

    const double CurrentTime = World->GetTimeSeconds();
    const double SafeCooldown =
        FMath::Max(1.0f, WaveSpawnCooldown);

    // 직전 웨이브 이후 쿨타임이 지나지 않았다면 생성하지 않음
    if (LastWaveSpawnTime >= 0.0 &&
        CurrentTime - LastWaveSpawnTime < SafeCooldown)
    {
        return false;
    }

    const int32 WaveSize =
        GetWaveSizeForCurrentState();

    if (WaveSize <= 0 || WaveSpawners.IsEmpty())
    {
        return false;
    }

    /*
     * 항상 첫 번째 스포너만 사용하지 않도록
     * 무작위 위치부터 순서대로 검사합니다.
     *
     * 선택된 스포너가 최대 생존 수에 도달했다면
     * 다음 스포너에서 생성을 시도합니다.
     */
    const int32 FirstSpawnerIndex =
        FMath::RandRange(0, WaveSpawners.Num() - 1);

    for (int32 Attempt = 0;
         Attempt < WaveSpawners.Num();
         ++Attempt)
    {
        const int32 SpawnerIndex =
            (FirstSpawnerIndex + Attempt) %
            WaveSpawners.Num();

        ABaruControlRoomSpawner* Spawner =
            WaveSpawners[SpawnerIndex];

        if (!IsValid(Spawner) ||
            !Spawner->IsSpawningEnabled())
        {
            continue;
        }

        const int32 SpawnedCount =
            Spawner->SpawnWave(
                WaveSize,
                TargetPlayer
            );

        if (SpawnedCount <= 0)
        {
            continue;
        }

        // 한 마리 이상 실제로 생성된 경우에만
        // 다음 웨이브를 막는 쿨타임 시작
        LastWaveSpawnTime = CurrentTime;

        BARU_NET_LOG(
            this,
            LogBaruAI,
            Log,
            TEXT(
                "Director wave spawned: "
                "Target=%s, State=%d, Requested=%d, Spawned=%d"
            ),
            *GetNameSafe(TargetPlayer),
            static_cast<int32>(CurrentDirectorState),
            WaveSize,
            SpawnedCount
        );

        return true;
    }

    return false;
}

void ABaruMonsterDirector::UpdateMonsterAssignments()
{
    // 디렉터 자동 판단은 서버에서만 실행
    if (GetNetMode() == NM_Client)
    {
        return;
    }

    // 파괴된 몬스터와 유효하지 않은 플레이어 기록 정리
    RemoveInvalidMonsters();
    RemoveInvalidPlayerThreats();
    RemoveInvalidAssignments();
    RefreshEncirclementAssignment();

    // 기존과 동일하게 타이머 간격만큼
    // 모든 플레이어의 위협도를 자연 감소
    const float ThreatDecayDeltaSeconds =
        FMath::Max(0.1f, AssignmentUpdateInterval);

    DecayPlayerThreats(ThreatDecayDeltaSeconds);

    // 완화 상태에서는 기존 위협도 감소만 처리하고
    // 새로운 웨이브는 만들지 않음
    if (CurrentDirectorState == EBaruDirectorState::Relief)
    {
        return;
    }

    APawn* HighestThreatPlayer = nullptr;
    float HighestThreat = 0.0f;

    /*
    * 여러 플레이어 중 현재 위협도가 가장 높은
    * 살아 있는 플레이어를 이번 웨이브 대상으로 선택합니다.
    * 탈출 저지 상태에서는 일반 위협도보다
    * 엘리베이터 밖에 남은 플레이어를 먼저 선택합니다.
    */
    if (CurrentDirectorState == EBaruDirectorState::Extraction)
    {
        APawn* ExtractionTarget =
            ExtractionTargetPlayer.Get();

        const ABaruPlayerState* ExtractionTargetState =
            IsValid(ExtractionTarget)
                ? ExtractionTarget->
                    GetPlayerState<ABaruPlayerState>()
                : nullptr;

        if (IsValid(ExtractionTarget) &&
            ExtractionTarget->IsPlayerControlled() &&
            IsValid(ExtractionTargetState) &&
            ExtractionTargetState->IsAlive())
        {
            HighestThreatPlayer = ExtractionTarget;
        }
    }

    /*
     * 탈출 목표가 없을 때는 기존 방식대로
     * 위협도가 가장 높은 살아 있는 플레이어를 선택합니다.
     */
    if (!IsValid(HighestThreatPlayer))
    {
        for (const TPair<TWeakObjectPtr<APawn>, float>& Entry :
             PlayerThreatScores)
        {
            APawn* PlayerPawn = Entry.Key.Get();

            if (!IsValid(PlayerPawn) ||
                !PlayerPawn->IsPlayerControlled())
            {
                continue;
            }

            const ABaruPlayerState* PlayerState =
                PlayerPawn->GetPlayerState<ABaruPlayerState>();

            if (!IsValid(PlayerState) ||
                !PlayerState->IsAlive())
            {
                continue;
            }

            if (Entry.Value <= HighestThreat)
            {
                continue;
            }

            HighestThreat = Entry.Value;
            HighestThreatPlayer = PlayerPawn;
        }
    }

    if (!IsValid(HighestThreatPlayer))
    {
        return;
    }

    /*
    * 일반 상태에서는 기존 위협도 조건을 사용합니다.
    *
    * 탈출 저지 상태에서는 공격 기록이 없는 플레이어라도
    * 엘리베이터 밖에 남아 있다면 웨이브를 생성합니다.
    */
    if (CurrentDirectorState != EBaruDirectorState::Extraction &&
        GetDesiredMonsterCount(HighestThreatPlayer) <= 0)
    {
        return;
    }
    
    // 직접 추적 몬스터는 유지하고
    // 다른 한 마리에게 우회·차단 명령 시도
    if (TryAssignEncirclement(HighestThreatPlayer))
    {
        // 포위와 웨이브가 동시에 시작되지 않도록
        // 이번 전술 배정을 공격 이벤트로 취급
        if (UWorld* World = GetWorld())
        {
            LastWaveSpawnTime = World->GetTimeSeconds();
        }

        BARU_NET_LOG(
            this,
            LogBaruAI,
            Log,
            TEXT(
                "Director selected encirclement: Target=%s"
            ),
            *GetNameSafe(HighestThreatPlayer)
        );

        return;
    }

    // 이번 테스트에서는 차단 담당이 유지되는 동안
    // 추가 매복이나 웨이브를 시작하지 않음
    if (CurrentEncirclementBlocker.IsValid())
    {
        return;
    }

    /*
    * 평상시 또는 압박 상태에서는 웨이브를 생성하기 전에
    * 기존 등록 몬스터 중 매복 가능한 개체가 있는지 확인
    *
    * 완화 상태는 위에서 이미 반환되고,
    * 탈출 저지 상태에서는 TryAssignAmbush가 false를 반환하므로
    * 기존 탈출 웨이브가 그대로 실행됨
    */
    if (TryAssignAmbush(HighestThreatPlayer))
    {
        /*
         * 매복과 새로운 웨이브가 짧은 간격으로 연속 발생하면
         * 플레이어가 한꺼번에 과도한 압박을 받을 수 있음
         *
         * 성공한 매복을 이번 디렉터의 공격 이벤트로 간주하여
         * 기존 웨이브 쿨타임도 함께 시작
         */
        if (UWorld* World = GetWorld())
        {
            LastWaveSpawnTime = World->GetTimeSeconds();
        }

        BARU_NET_LOG(
            this,
            LogBaruAI,
            Log,
            TEXT(
                "Director selected ambush instead of wave: "
                "Target=%s"
            ),
            *GetNameSafe(HighestThreatPlayer)
        );

        return;
    }

    // 매복을 선택하지 않았거나 적합한 몬스터가 없다면
    // 지정된 스포너에서 기존 방식대로 새로운 웨이브 생성
    TrySpawnDirectorWave(HighestThreatPlayer);
}

bool ABaruMonsterDirector::TryAssignAmbush(
    APawn* TargetPlayer
)
{
    // 매복 판단은 서버에서만 수행
    if (GetNetMode() == NM_Client ||
        !IsValid(TargetPlayer) ||
        !TargetPlayer->IsPlayerControlled())
    {
        return false;
    }

    // 완화 상태에서는 팀이 회복할 시간을 주고,
    // 탈출 저지 상태에서는 숨지 않고 직접 돌진
    if (CurrentDirectorState == EBaruDirectorState::Relief ||
        CurrentDirectorState == EBaruDirectorState::Extraction)
    {
        return false;
    }

    if (MaxConcurrentAmbushers <= 0)
    {
        return false;
    }

    UWorld* World = GetWorld();

    if (!World)
    {
        return false;
    }

    const double CurrentTime = World->GetTimeSeconds();

    // 이전 판단 이후 쿨타임이 지나지 않았다면 재시도하지 않음
    if (LastAmbushDecisionTime >= 0.0 &&
        CurrentTime - LastAmbushDecisionTime <
            AmbushDecisionCooldown)
    {
        return false;
    }

    // 후보가 없거나 확률에 실패하더라도
    // 매 타이머마다 반복 검사하지 않도록 판단 시간을 기록
    LastAmbushDecisionTime = CurrentTime;

    const float AmbushChance =
        CurrentDirectorState == EBaruDirectorState::Pressure
            ? PressureAmbushChance
            : NormalAmbushChance;

    // 현재 상태에 설정된 매복 확률 검사
    if (FMath::FRand() >
        FMath::Clamp(AmbushChance, 0.0f, 1.0f))
    {
        return false;
    }

    RemoveInvalidMonsters();

    int32 ActiveAmbusherCount = 0;

    // 이미 매복 명령을 수행 중인 몬스터 수 확인
    for (const TWeakObjectPtr<ABaruMonsterCharacter>& Entry :
         RegisteredMonsters)
    {
        ABaruMonsterCharacter* Monster = Entry.Get();

        if (!IsValid(Monster))
        {
            continue;
        }

        const ABaruMonsterAIController* MonsterController =
            Cast<ABaruMonsterAIController>(
                Monster->GetController()
            );

        if (IsValid(MonsterController) &&
            MonsterController->GetDirectorCommand() ==
                EBaruMonsterDirectorCommand::Ambush)
        {
            ++ActiveAmbusherCount;
        }
    }

    if (ActiveAmbusherCount >= MaxConcurrentAmbushers)
    {
        return false;
    }

    ABaruMonsterCharacter* BestCandidate = nullptr;
    double BestDistanceSquared =
        TNumericLimits<double>::Max();

    const double MinimumDistanceSquared =
        FMath::Square(
            static_cast<double>(
                FMath::Max(0.0f, MinimumAmbushDistance)
            )
        );

    // 매복 가능한 몬스터 중 목표와 가장 가까운 후보 선택
    for (const TWeakObjectPtr<ABaruMonsterCharacter>& Entry :
         RegisteredMonsters)
    {
        ABaruMonsterCharacter* Monster = Entry.Get();

        if (!IsValid(Monster) ||
            ICombatInterface::Execute_IsDead(Monster))
        {
            continue;
        }

        const UBaruMonsterDataAsset* MonsterData =
            Monster->GetMonsterDataAsset();

        if (!IsValid(MonsterData) ||
            !MonsterData->bCanAmbush)
        {
            continue;
        }

        ABaruMonsterAIController* MonsterController =
            Cast<ABaruMonsterAIController>(
                Monster->GetController()
            );

        if (!IsValid(MonsterController) ||
            !MonsterController->HasAuthority())
        {
            continue;
        }

        // 다른 디렉터 명령을 수행 중인 몬스터는 방해하지 않음
        if (MonsterController->GetDirectorCommand() !=
            EBaruMonsterDirectorCommand::None)
        {
            continue;
        }

        // 현재 플레이어를 추적하거나 마지막 위치를
        // 조사 중인 몬스터는 전투에서 빼내지 않음
        if (IsValid(MonsterController->GetCurrentTarget()) ||
            MonsterController->HasLastKnownTargetLocation())
        {
            continue;
        }

        const double DistanceSquared =
            FVector::DistSquared(
                Monster->GetActorLocation(),
                TargetPlayer->GetActorLocation()
            );

        // 너무 가까운 몬스터가 부자연스럽게 도망쳐
        // 숨는 행동을 하지 않도록 제외
        if (DistanceSquared < MinimumDistanceSquared)
        {
            continue;
        }

        if (DistanceSquared < BestDistanceSquared)
        {
            BestDistanceSquared = DistanceSquared;
            BestCandidate = Monster;
        }
    }

    if (!IsValid(BestCandidate))
    {
        return false;
    }

    return RequestAmbush(
        BestCandidate,
        TargetPlayer
    );
}

void ABaruMonsterDirector::
    RefreshEncirclementAssignment()
{
    ABaruMonsterCharacter* Blocker =
        CurrentEncirclementBlocker.Get();

    APawn* TargetPlayer =
        CurrentEncirclementTarget.Get();

    ABaruMonsterTacticalRoute* TacticalRoute =
        CurrentEncirclementRoute.Get();

    // 저장된 배정이 전혀 없다면 초기 상태 유지
    if (!IsValid(Blocker) &&
        !IsValid(TargetPlayer) &&
        !IsValid(TacticalRoute))
    {
        CurrentEncirclementBlocker.Reset();
        CurrentEncirclementTarget.Reset();
        CurrentEncirclementRoute.Reset();
        return;
    }

    ABaruMonsterAIController* BlockerController =
        IsValid(Blocker)
            ? Cast<ABaruMonsterAIController>(
                Blocker->GetController()
            )
            : nullptr;

    const ABaruPlayerState* TargetPlayerState =
        IsValid(TargetPlayer)
            ? TargetPlayer->
                GetPlayerState<ABaruPlayerState>()
            : nullptr;

    const bool bValidBlocker =
        IsValid(Blocker) &&
        !ICombatInterface::Execute_IsDead(Blocker) &&
        RegisteredMonsters.Contains(
            TWeakObjectPtr<ABaruMonsterCharacter>(Blocker)
        );

    const bool bValidTarget =
        IsValid(TargetPlayer) &&
        TargetPlayer->IsPlayerControlled() &&
        IsValid(TargetPlayerState) &&
        TargetPlayerState->IsAlive();

    const bool bRelatedCommand =
        IsValid(BlockerController) &&
        (
            BlockerController->GetDirectorCommand() ==
                EBaruMonsterDirectorCommand::Encircle ||
            BlockerController->GetDirectorCommand() ==
                EBaruMonsterDirectorCommand::Hold
        );

    const bool bValidRoute =
        IsValid(TacticalRoute) &&
        bValidBlocker &&
        TacticalRoute->IsRouteConfigured() &&
        TacticalRoute->IsAvailableFor(Blocker);

    bool bTargetStillNearRoute = false;

    if (bValidTarget && bValidRoute)
    {
        // 활성화 반경보다 약간 넓은 범위를 유지 반경으로 사용
        // 경계에서 명령이 계속 켜졌다 꺼지는 현상을 방지
        const double RetentionRadius =
            static_cast<double>(
                FMath::Max(
                    100.0f,
                    EncirclementRouteActivationRadius
                ) * 1.5f
            );

        bTargetStillNearRoute =
            FVector::DistSquared(
                TargetPlayer->GetActorLocation(),
                TacticalRoute->GetBlockLocation()
            ) <= FMath::Square(RetentionRadius);
    }

    const bool bMatchesExtractionTarget =
        CurrentDirectorState !=
            EBaruDirectorState::Extraction ||
        !ExtractionTargetPlayer.IsValid() ||
        ExtractionTargetPlayer.Get() == TargetPlayer;

    // 압박·탈출 저지 상태이며 나머지 조건도 유효해야 포위 유지
    const bool bCanContinue =
        IsEncirclementAllowed() &&
        bValidBlocker &&
        bValidTarget &&
        bRelatedCommand &&
        bValidRoute &&
        bTargetStillNearRoute &&
        bMatchesExtractionTarget;

    if (bCanContinue)
    {
        return;
    }

    // 몬스터가 아직 포위 또는 차단 대기 명령을
    // 수행 중이라면 기존 명령도 함께 해제
    if (IsValid(BlockerController) &&
        BlockerController->HasAuthority() &&
        bRelatedCommand)
    {
        BlockerController->ClearDirectorCommand();
    }

    CurrentEncirclementBlocker.Reset();
    CurrentEncirclementTarget.Reset();
    CurrentEncirclementRoute.Reset();
}

bool ABaruMonsterDirector::TryAssignEncirclement(
    APawn* TargetPlayer
)
{
    // Pressure와 Extraction에서만 자동 포위 배정을 시도
    if (!IsEncirclementAllowed() ||
        GetNetMode() == NM_Client ||
        !IsValid(TargetPlayer) ||
        !TargetPlayer->IsPlayerControlled() ||
        TacticalRoutes.IsEmpty())
    {
        return false;
    }

    // 현재 한 명이 우회 또는 차단 위치 점유 중이면
    // 추가 포위 담당자를 배정하지 않음
    if (CurrentEncirclementBlocker.IsValid())
    {
        return false;
    }

    const ABaruPlayerState* TargetPlayerState =
        TargetPlayer->GetPlayerState<ABaruPlayerState>();

    if (!IsValid(TargetPlayerState) ||
        !TargetPlayerState->IsAlive())
    {
        return false;
    }

    UWorld* World = GetWorld();

    if (!IsValid(World))
    {
        return false;
    }

    const double CurrentTime = World->GetTimeSeconds();
    const double SafeDecisionCooldown =
        FMath::Max(
            1.0f,
            EncirclementDecisionCooldown
        );

    if (LastEncirclementDecisionTime >= 0.0 &&
        CurrentTime - LastEncirclementDecisionTime <
            SafeDecisionCooldown)
    {
        return false;
    }

    RemoveInvalidMonsters();

    int32 RelevantParticipantCount = 0;

    // 가장 가까이서 플레이어를 직접 쫓는 몬스터는
    // 압박 담당으로 남기고 우회 후보에서 제외
    ABaruMonsterCharacter* PressureMonster = nullptr;

    double ClosestPressureDistanceSquared =
        TNumericLimits<double>::Max();

    for (const TWeakObjectPtr<ABaruMonsterCharacter>& Entry :
         RegisteredMonsters)
    {
        ABaruMonsterCharacter* Monster = Entry.Get();

        if (!IsValid(Monster) ||
            ICombatInterface::Execute_IsDead(Monster))
        {
            continue;
        }

        ABaruMonsterAIController* MonsterController =
            Cast<ABaruMonsterAIController>(
                Monster->GetController()
            );

        if (!IsValid(MonsterController) ||
            !MonsterController->HasAuthority())
        {
            continue;
        }

        const EBaruMonsterDirectorCommand Command =
            MonsterController->GetDirectorCommand();

        // 다른 경로에서 이미 포위 명령을 수행 중이라면
        // 동시에 두 번째 포위를 시작하지 않음
        if (Command ==
            EBaruMonsterDirectorCommand::Encircle)
        {
            return false;
        }

        APawn* MonsterTarget =
            MonsterController->GetCurrentTarget();

        const bool bCanApplyDirectPressure =
            Command ==
                EBaruMonsterDirectorCommand::None ||
            Command ==
                EBaruMonsterDirectorCommand::Hold;

        const bool bPursuingTarget =
            bCanApplyDirectPressure &&
            MonsterTarget == TargetPlayer;

        const bool bIdleAndAvailable =
            Command ==
                EBaruMonsterDirectorCommand::None &&
            !IsValid(MonsterTarget) &&
            !MonsterController->
                HasLastKnownTargetLocation();

        if (!bPursuingTarget &&
            !bIdleAndAvailable)
        {
            continue;
        }

        ++RelevantParticipantCount;

        if (!bPursuingTarget)
        {
            continue;
        }

        const double DistanceSquared =
            FVector::DistSquared(
                Monster->GetActorLocation(),
                TargetPlayer->GetActorLocation()
            );

        if (DistanceSquared <
            ClosestPressureDistanceSquared)
        {
            PressureMonster = Monster;
            ClosestPressureDistanceSquared =
                DistanceSquared;
        }
    }

    // 플레이어를 직접 압박하는 몬스터가 반드시 한 마리 필요
    // 나머지 한 마리가 우회 역할을 담당
    if (!IsValid(PressureMonster) ||
        RelevantParticipantCount <
            FMath::Max(
                2,
                MinimumEncirclementParticipants
            ))
    {
        return false;
    }

    // 실제 후보와 경로를 검사하기 시작한 시점을 기록
    LastEncirclementDecisionTime = CurrentTime;

    UNavigationSystemV1* NavigationSystem =
        FNavigationSystem::GetCurrent<
            UNavigationSystemV1
        >(World);

    if (!IsValid(NavigationSystem))
    {
        return false;
    }

    ABaruMonsterCharacter* BestMonster = nullptr;
    ABaruMonsterTacticalRoute* BestRoute = nullptr;

    float BestTotalPathLength =
        TNumericLimits<float>::Max();

    const double MaximumCandidateDistanceSquared =
        FMath::Square(
            static_cast<double>(
                FMath::Max(
                    100.0f,
                    MaximumEncirclementCandidateDistance
                )
            )
        );

    const double RouteActivationRadiusSquared =
        FMath::Square(
            static_cast<double>(
                FMath::Max(
                    100.0f,
                    EncirclementRouteActivationRadius
                )
            )
        );

    const FVector ProjectionExtent(
        150.0f,
        150.0f,
        250.0f
    );

    for (const TWeakObjectPtr<ABaruMonsterCharacter>& Entry :
         RegisteredMonsters)
    {
        ABaruMonsterCharacter* Monster = Entry.Get();

        if (!IsValid(Monster) ||
            Monster == PressureMonster ||
            ICombatInterface::Execute_IsDead(Monster))
        {
            continue;
        }

        ABaruMonsterAIController* MonsterController =
            Cast<ABaruMonsterAIController>(
                Monster->GetController()
            );

        if (!IsValid(MonsterController) ||
            !MonsterController->HasAuthority() ||
            MonsterController->GetDirectorCommand() !=
                EBaruMonsterDirectorCommand::None)
        {
            continue;
        }

        APawn* MonsterTarget =
            MonsterController->GetCurrentTarget();

        // 다른 플레이어와 싸우는 몬스터는 데려오지 않음
        if (IsValid(MonsterTarget) &&
            MonsterTarget != TargetPlayer)
        {
            continue;
        }

        // 다른 플레이어의 마지막 위치를 수색 중인
        // 몬스터도 포위 후보에서 제외
        if (!IsValid(MonsterTarget) &&
            MonsterController->
                HasLastKnownTargetLocation())
        {
            continue;
        }

        if (FVector::DistSquared(
                Monster->GetActorLocation(),
                TargetPlayer->GetActorLocation()
            ) > MaximumCandidateDistanceSquared)
        {
            continue;
        }

        for (
            const TObjectPtr<
                ABaruMonsterTacticalRoute
            >& RouteEntry :
            TacticalRoutes
        )
        {
            ABaruMonsterTacticalRoute* TacticalRoute =
                RouteEntry.Get();

            if (!IsValid(TacticalRoute) ||
                !TacticalRoute->
                    IsAvailableFor(Monster))
            {
                continue;
            }

            const FVector RouteEntryLocation =
                TacticalRoute->
                    GetRouteEntryLocation();

            const FVector BlockLocation =
                TacticalRoute->
                    GetBlockLocation();

            // 플레이어가 이 경로가 담당하는 차단 구역
            // 근처에 있을 때만 사용
            if (FVector::DistSquared(
                    TargetPlayer->GetActorLocation(),
                    BlockLocation
                ) > RouteActivationRadiusSquared)
            {
                continue;
            }

            FNavLocation ProjectedRouteEntry;
            FNavLocation ProjectedBlockLocation;

            if (!NavigationSystem->
                    ProjectPointToNavigation(
                        RouteEntryLocation,
                        ProjectedRouteEntry,
                        ProjectionExtent
                    ) ||
                !NavigationSystem->
                    ProjectPointToNavigation(
                        BlockLocation,
                        ProjectedBlockLocation,
                        ProjectionExtent
                    ))
            {
                continue;
            }

            UNavigationPath* PathToRouteEntry =
                UNavigationSystemV1::
                    FindPathToLocationSynchronously(
                        World,
                        Monster->GetActorLocation(),
                        ProjectedRouteEntry.Location,
                        MonsterController,
                        nullptr
                    );

            if (!IsValid(PathToRouteEntry) ||
                !PathToRouteEntry->IsValid() ||
                PathToRouteEntry->IsPartial())
            {
                continue;
            }

            UNavigationPath* PathToBlock =
                UNavigationSystemV1::
                    FindPathToLocationSynchronously(
                        World,
                        ProjectedRouteEntry.Location,
                        ProjectedBlockLocation.Location,
                        MonsterController,
                        nullptr
                    );

            if (!IsValid(PathToBlock) ||
                !PathToBlock->IsValid() ||
                PathToBlock->IsPartial())
            {
                continue;
            }

            const float TotalPathLength =
                PathToRouteEntry->GetPathLength() +
                PathToBlock->GetPathLength();

            if (!FMath::IsFinite(TotalPathLength) ||
                TotalPathLength >= BestTotalPathLength)
            {
                continue;
            }

            BestTotalPathLength = TotalPathLength;
            BestMonster = Monster;
            BestRoute = TacticalRoute;
        }
    }

    if (!IsValid(BestMonster) ||
        !IsValid(BestRoute))
    {
        return false;
    }

    if (!RequestEncirclement(
            BestMonster,
            TargetPlayer,
            BestRoute
        ))
    {
        return false;
    }

    CurrentEncirclementBlocker = BestMonster;
    CurrentEncirclementTarget = TargetPlayer;
    CurrentEncirclementRoute = BestRoute;

    return true;
}

bool ABaruMonsterDirector::IsEncirclementAllowed() const
{
    // Normal과 Relief에서는 포위를 시작하거나 유지하지 않음
    return
        CurrentDirectorState ==
            EBaruDirectorState::Pressure ||
        CurrentDirectorState ==
            EBaruDirectorState::Extraction;
}