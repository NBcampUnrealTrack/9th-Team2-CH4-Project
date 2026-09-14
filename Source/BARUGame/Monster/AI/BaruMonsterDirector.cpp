


#include "Monster/AI/BaruMonsterDirector.h"

#include "Monster/Characters/BaruMonsterCharacter.h"
#include "Monster/AI/BaruMonsterAIController.h"
#include "Interfaces/CombatInterface.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameStateBase.h"
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

void ABaruMonsterDirector::UpdateMonsterAssignments()
{
    // 몬스터 배정은 서버에서만 수행
    if (GetNetMode() == NM_Client)
    {
        return;
    }

    // 배정 계산 전에 파괴·사망한 대상과
    // 오래된 플레이어 위협도 기록을 정리
    RemoveInvalidMonsters();
    RemoveInvalidPlayerThreats();
    RemoveInvalidAssignments();

    // 각 플레이어에게 현재 몇 마리를 유지했는지 기록
    TMap<TWeakObjectPtr<APawn>, int32> KeptAssignmentCounts;

    // -------------------------------------------------------------------------
    // 1. 현재 목표 배정 수보다 많아진 몬스터 해제
    // -------------------------------------------------------------------------
    for (auto It = MonsterAssignments.CreateIterator(); It; ++It)
    {
        ABaruMonsterCharacter* Monster =
            It.Key().Get();

        APawn* PlayerPawn =
            It.Value().Get();

        const int32 DesiredCount =
            GetDesiredMonsterCount(PlayerPawn);

        int32& KeptCount =
            KeptAssignmentCounts.FindOrAdd(It.Value());

        // 필요한 수만큼의 기존 배정은 그대로 유지
        if (KeptCount < DesiredCount)
        {
            KeptCount++;
            continue;
        }

        // 목표 배정 수를 초과한 몬스터의 기존 명령 해제
        if (ABaruMonsterAIController* MonsterController =
            FindCommandController(Monster))
        {
            MonsterController->ClearDirectorCommand();
        }

        BARU_NET_LOG(
            this,
            LogBaruAI,
            Log,
            TEXT("Monster assignment released: %s -> %s"),
            *GetNameSafe(Monster),
            *GetNameSafe(PlayerPawn)
        );

        // 초과 배정 기록 제거
        It.RemoveCurrent();
    }

    // -------------------------------------------------------------------------
    // 2. 목표 수보다 부족한 플레이어에게 새 몬스터 배정
    // -------------------------------------------------------------------------
    for (const TPair<TWeakObjectPtr<APawn>, float>& ThreatEntry :
         PlayerThreatScores)
    {
        APawn* PlayerPawn =
            ThreatEntry.Key.Get();

        if (!IsValid(PlayerPawn) ||
            !PlayerPawn->IsPlayerControlled())
        {
            continue;
        }

        // DBNO 또는 사망한 플레이어에게는
        // 새로운 몬스터를 배정하지 않음
        const ABaruPlayerState* PlayerState =
            PlayerPawn->GetPlayerState<ABaruPlayerState>();

        if (!IsValid(PlayerState) ||
            !PlayerState->IsAlive())
        {
            continue;
        }

        const int32 DesiredCount =
            GetDesiredMonsterCount(PlayerPawn);

        int32 CurrentCount =
            KeptAssignmentCounts.FindRef(ThreatEntry.Key);

        // 현재 배정 수가 목표 수에 도달할 때까지 반복
        while (CurrentCount < DesiredCount)
        {
            // 아직 배정되지 않은 몬스터 중
            // 플레이어에게 가장 가까운 개체 선택
            ABaruMonsterCharacter* Monster =
                FindClosestUnassignedMonster(PlayerPawn);

            // 배정할 수 있는 몬스터가 더 이상 없으면 종료
            if (!IsValid(Monster))
            {
                break;
            }

            // 플레이어의 현재 위치로 조사 명령 전달
            const bool bCommandAccepted =
                RequestInvestigation(
                    Monster,
                    PlayerPawn->GetActorLocation()
                );

            // 명령 전달에 실패하면 같은 몬스터를
            // 반복 선택하지 않도록 이번 플레이어 배정 종료
            if (!bCommandAccepted)
            {
                break;
            }

            const TWeakObjectPtr<ABaruMonsterCharacter> MonsterRef(
                Monster
            );

            // 몬스터와 플레이어의 배정 관계 기록
            MonsterAssignments.Add(
                MonsterRef,
                ThreatEntry.Key
            );

            CurrentCount++;
            KeptAssignmentCounts.FindOrAdd(ThreatEntry.Key) =
                CurrentCount;

            BARU_NET_LOG(
                this,
                LogBaruAI,
                Log,
                TEXT(
                    "Monster assigned: %s -> %s / Count=%d, Desired=%d"
                ),
                *GetNameSafe(Monster),
                *GetNameSafe(PlayerPawn),
                CurrentCount,
                DesiredCount
            );
        }
    }
}

