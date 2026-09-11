


#include "Monster/AI/BaruMonsterDirector.h"

#include "Monster/Characters/BaruMonsterCharacter.h"
#include "Monster/AI/BaruMonsterAIController.h"
#include "Interfaces/CombatInterface.h"
#include "BaruLog.h"



ABaruMonsterDirector::ABaruMonsterDirector()
{
	PrimaryActorTick.bCanEverTick = false;

	// 지휘 판단과 목록은 서버에서만 관리
	// 실제 몬스터의 이동·전투 결과는 몬스터 쪽에서 동기화
	bReplicates = false;
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
        // 명령 해제로 다른 콜백이 실행될 수 있으므로
        // 순회할 목록을 먼저 복사
        const TArray<TWeakObjectPtr<ABaruMonsterCharacter>> Monsters =
            RegisteredMonsters;

        // 디렉터의 원본 목록은 먼저 비움
        RegisteredMonsters.Reset();

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
