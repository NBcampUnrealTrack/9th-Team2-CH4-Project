#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"
#include "BaruMonsterDirector.generated.h"

class ABaruMonsterCharacter;
class ABaruMonsterAIController;
class ABaruMonsterTacticalRoute;
class ABaruControlRoomSpawner;
class APawn;

/**
 * 몬스터에게 명령을 전달하는 디렉터
 *
 * 현재 담당하는 기능:
 * 1. 지휘할 몬스터를 목록에 등록
 * 2. 등록된 몬스터에게 조사·대기·명령 해제 요청 전달
 * 3. 사망하거나 제거된 몬스터를 목록에서 정리
 *
 * 디렉터가 직접 몬스터를 이동시키지는 않음
 * Director → AIController → Blackboard → Behavior Tree 순서로 명령 전달
 *
 * GameMode의 몬스터 수·킬 집계와는 별개의 지휘 목록
 * 위협도·운영 상태에 따라 전술 배정과 웨이브 생성을 판단
 */

// 디렉터의 전체 운영 상태
// 개별 몬스터의 추적·공격 상태와는 별도로 관리
UENUM(BlueprintType)
enum class EBaruDirectorState : uint8
{
    // 기본적인 조사와 소규모 파견
    Normal UMETA(DisplayName = "평상시"),

    // 지원 수나 파견 빈도를 높이는 상태
    Pressure UMETA(DisplayName = "압박"),

    // 플레이어가 회복할 수 있도록 추가 파견을 줄이는 상태
    Relief UMETA(DisplayName = "완화"),

    // 엘리베이터 카운트다운 중 탈출 저지 규칙을 적용
    Extraction UMETA(DisplayName = "탈출 저지")
};


UCLASS()
class BARUGAME_API ABaruMonsterDirector : public AActor
{
    GENERATED_BODY()

public:
    
    ABaruMonsterDirector();
    
    // 살아 있는 몬스터를 지휘 대상에 등록
    // 이미 등록된 몬스터는 중복 추가하지 않음
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Monster|Director")
    bool RegisterMonster(ABaruMonsterCharacter* Monster);

    // 사망하거나 제거되는 몬스터를 지휘 목록에서 제외
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Monster|Director")
    void UnregisterMonster(ABaruMonsterCharacter* Monster);

    // 등록된 몬스터 한 마리에게 조사 명령 전달
    // true는 명령 접수 성공이며 도착 성공을 의미하지 않음
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Monster|Director")
    bool RequestInvestigation(
       ABaruMonsterCharacter* Monster,
       const FVector& TargetLocation
    );
    
    // 등록된 몬스터 한 마리에게 특정 플레이어 매복 명령 전달
    //
    // 디렉터는 매복 대상만 지정하고,
    // 실제 은폐 위치 탐색과 이동은 개별 AI가 처리
    UFUNCTION(
       BlueprintCallable,
       BlueprintAuthorityOnly,
       Category = "Monster|Director"
    )
    bool RequestAmbush(
       ABaruMonsterCharacter* Monster,
       APawn* TargetPlayer
    );
    
    // 지정한 몬스터에게 전술 경로를 이용한
    // 측면 우회 및 퇴로 차단 명령 전달
    UFUNCTION(
       BlueprintCallable,
       BlueprintAuthorityOnly,
       Category = "Monster|Director"
    )
    bool RequestEncirclement(
       ABaruMonsterCharacter* Monster,
       APawn* TargetPlayer,
       ABaruMonsterTacticalRoute* TacticalRoute
    );

    // 등록된 몬스터 한 마리에게 대기 명령 전달
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Monster|Director")
    bool RequestHold(ABaruMonsterCharacter* Monster);

    // 등록된 몬스터의 디렉터 명령 해제
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Monster|Director")
    bool RequestClearCommand(ABaruMonsterCharacter* Monster);
    
    // =========================================================================
    // 플레이어별 디렉터 위협도
    // =========================================================================

    // 해당 플레이어의 위협도를 지정한 값으로 설정
    // 서버에서만 처리하며 0~100 범위로 제한
    // 0으로 설정하면 저장 목록에서 제거
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Monster|Director|Threat")
    void SetPlayerThreat(
       APawn* PlayerPawn,
       float NewThreat
    );

    // 현재 위협도에 변화량을 더함
    // 양수는 증가, 음수는 감소
    // 아직 기록이 없으면 0점에서 시작
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Monster|Director|Threat")
    void AddPlayerThreat(
       APawn* PlayerPawn,
       float ThreatDelta
    );
    
    // 플레이어가 몬스터에게 피해를 줬을 때 호출
    // 실제 피해량을 디렉터 위협도로 변환하여 누적
    UFUNCTION(
       BlueprintCallable,
       BlueprintAuthorityOnly,
       Category = "Monster|Director|Threat"
    )
    void ReportPlayerDamageThreat(
       APawn* PlayerPawn,
       float DamageAmount
    );

    // 저장된 위협도를 반환
    // 기록이 없거나 유효하지 않은 플레이어라면 0 반환
    // 현재 목록은 복제하지 않으므로 서버에서 조회할 때 사용
    UFUNCTION(BlueprintPure, Category = "Monster|Director|Threat")
    float GetPlayerThreat(APawn* PlayerPawn) const;
    
    // 플레이어의 현재 위협도를 기준으로 목표 배정 수를 계산
    // 실제 몬스터 선택이나 명령 전달은 별도 단계에서 처리
    UFUNCTION(BlueprintPure, Category = "Monster|Director|Threat")
    int32 GetDesiredMonsterCount(APawn* PlayerPawn) const;
    
    // 서버에서 디렉터의 운영 상태를 변경
    // 이후 StateTree의 상태 진입 처리에서 호출
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Monster|Director|State")
    void SetDirectorState(EBaruDirectorState NewState);

    // 현재 운영 상태 조회
    // 현재 디렉터 상태는 복제하지 않으므로 서버 판단에 사용
    UFUNCTION(BlueprintPure, Category = "Monster|Director|State")
    EBaruDirectorState GetDirectorState() const
    {
       return CurrentDirectorState;
    }
    
    // =========================================================================
    // 팀 전체 부담도
    // =========================================================================

    // 현재 팀이 얼마나 벅찬 상태인지 직접 설정
    // 0은 매우 여유로운 상태, 100은 매우 위험한 상태
    UFUNCTION(
       BlueprintCallable,
       BlueprintAuthorityOnly,
       Category = "Monster|Director|Team Burden"
    )
    void SetTeamBurden(float NewBurden);

    // 현재 팀 부담도에 변화량을 더함
    // 양수는 부담 증가, 음수는 부담 감소
    UFUNCTION(
       BlueprintCallable,
       BlueprintAuthorityOnly,
       Category = "Monster|Director|Team Burden"
    )
    void AddTeamBurden(float BurdenDelta);

    // 현재 팀 부담도 반환
    UFUNCTION(
       BlueprintPure,
       Category = "Monster|Director|Team Burden"
    )
    float GetTeamBurden() const
    {
       return TeamBurden;
    }
    
    // 일부 플레이어가 엘리베이터 밖에 남은 상태로
    // 출발 카운트다운이 진행 중인지 설정
    UFUNCTION(
       BlueprintCallable,
       BlueprintAuthorityOnly,
       Category = "Monster|Director|Extraction"
    )
    void SetExtractionActive(bool bNewExtractionActive);
    
    /**
    * 탈출 저지 웨이브가 향할 플레이어를 설정합니다.
    *
    * 엘리베이터 카운트다운 중 외부에 남아 있는
    * 살아 있는 플레이어가 대상으로 전달됩니다.
    *
     * nullptr를 전달하면 기존 목표를 제거합니다.
     */
    UFUNCTION(
       BlueprintCallable,
       BlueprintAuthorityOnly,
       Category = "Monster|Director|Extraction"
    )
    void SetExtractionTarget(APawn* TargetPlayer);

    // 카운트다운 중 탑승 위치와 남은 시간을 함께 전달
    void UpdateExtractionContext(
        AActor* SourceElevator,
        APawn* TargetPlayer,
        const FVector& BoardingLocation,
        float SecondsRemaining
    );

    // 요청한 엘리베이터의 탈출 저지 정보만 해제
    void ClearExtractionContext(AActor* SourceElevator);

    // 현재 탈출 저지 상태가 필요한지 반환
    UFUNCTION(
       BlueprintPure,
       Category = "Monster|Director|Extraction"
    )
    bool IsExtractionActive() const
    {
       return bExtractionActive;
    }

protected:
    
    // 플레이어 상태가 준비된 후 팀 부담도 자동 계산을 시작
    virtual void BeginPlay() override;
    
    virtual void EndPlay(
       const EEndPlayReason::Type EndPlayReason
   ) override;
    
    // 몬스터 한 마리를 추가 배정하는 데 필요한 위협도
    // 기본값 25:
    // 25점이면 1마리, 50점이면 2마리, 75점이면 3마리
    UPROPERTY(
       EditDefaultsOnly,
       BlueprintReadOnly,
       Category = "Monster|Director|Allocation",
       meta = (ClampMin = "1.0")
    )
    float ThreatPerMonster = 25.0f;
    
    // 플레이어가 몬스터에게 준 피해 1당 증가하는 디렉터 위협도
    //
    // 기본값 1:
    // 피해 30을 주면 위협도도 30 증가
    UPROPERTY(
       EditDefaultsOnly,
       BlueprintReadOnly,
       Category = "Monster|Director|Threat",
       meta = (ClampMin = "0.0")
    )
    float ThreatGainPerDamage = 1.0f;

    // 전투가 끝난 뒤 1초마다 감소하는 플레이어 위협도
    //
    // 기본값 2:
    // 위협도 50이라면 추가 행동이 없을 때 약 25초 후 0이 됨
    UPROPERTY(
       EditDefaultsOnly,
       BlueprintReadOnly,
       Category = "Monster|Director|Threat",
       meta = (ClampMin = "0.0")
    )
    float PlayerThreatDecayPerSecond = 2.0f;

    // 디렉터가 플레이어 한 명에게 배정할 수 있는 최대 수
    // 0이면 해당 디렉터의 자동 배정을 하지 않음
    //
    // 개별 AI가 직접 발견해서 추적하는 몬스터까지
    // 이 값으로 강제로 제한하는 것은 아님
    UPROPERTY(
       EditDefaultsOnly,
       BlueprintReadOnly,
       Category = "Monster|Director|Allocation",
       meta = (ClampMin = "0")
    )
    int32 MaxMonstersPerPlayer = 3;
    
    // 플레이어 배정 상태를 다시 확인하는 간격
    // Actor Tick 대신 타이머로 처리
    UPROPERTY(
       EditDefaultsOnly,
       BlueprintReadOnly,
       Category = "Monster|Director|Allocation",
       meta = (ClampMin = "0.1")
    )
    float AssignmentUpdateInterval = 2.0f;
    
    // =========================================================================
    // 디렉터 웨이브 스폰 설정
    // =========================================================================

    /**
     * 디렉터가 사용할 웨이브 스포너 목록
     *
     * 레벨에 배치된 MonsterDirector 인스턴스에서
     * 사용할 스포너들을 직접 지정합니다.
     */
    UPROPERTY(
       EditInstanceOnly,
       BlueprintReadOnly,
       Category = "Monster|Director|Spawning"
    )
    TArray<TObjectPtr<ABaruControlRoomSpawner>> WaveSpawners;

    /**
     * 웨이브가 반복 생성되는 것을 막는 전역 쿨타임
     *
     * 플레이어 위협도가 계속 높더라도
     * 이 시간이 지나기 전에는 다음 웨이브를 생성하지 않습니다.
     */
    UPROPERTY(
       EditAnywhere,
       BlueprintReadOnly,
       Category = "Monster|Director|Spawning",
       meta = (ClampMin = "1.0")
    )
    float WaveSpawnCooldown = 15.0f;

    /** 평상시 한 번에 생성할 몬스터 수 */
    UPROPERTY(
       EditAnywhere,
       BlueprintReadOnly,
       Category = "Monster|Director|Spawning",
       meta = (ClampMin = "1")
    )
    int32 NormalWaveSize = 2;

    /** 압박 상태에서 한 번에 생성할 몬스터 수 */
    UPROPERTY(
       EditAnywhere,
       BlueprintReadOnly,
       Category = "Monster|Director|Spawning",
       meta = (ClampMin = "1")
    )
    int32 PressureWaveSize = 4;

    /** 탈출 저지 상태에서 한 번에 생성할 몬스터 수 */
    UPROPERTY(
       EditAnywhere,
       BlueprintReadOnly,
       Category = "Monster|Director|Spawning",
       meta = (ClampMin = "1")
    )
    int32 ExtractionWaveSize = 5;
    
    // =========================================================================
    // 디렉터 매복 설정
    // =========================================================================

    /**
     * 평상시 매복 명령을 시도할 확률
     *
     * 0.15는 15%, 1.0은 100%를 의미합니다.
     */
    UPROPERTY(
       EditAnywhere,
       BlueprintReadOnly,
       Category = "Monster|Director|Ambush",
       meta = (ClampMin = "0.0", ClampMax = "1.0")
    )
    float NormalAmbushChance = 0.15f;

    /**
     * 압박 상태에서 매복 명령을 시도할 확률
     */
    UPROPERTY(
       EditAnywhere,
       BlueprintReadOnly,
       Category = "Monster|Director|Ambush",
       meta = (ClampMin = "0.0", ClampMax = "1.0")
    )
    float PressureAmbushChance = 0.35f;

    /**
     * 매복 판단을 다시 시도하기까지 기다리는 시간
     *
     * 배정 성공 여부와 관계없이 적용하여
     * 매복 가능한 몬스터가 없을 때도 매 프레임 검사하지 않습니다.
     */
    UPROPERTY(
       EditAnywhere,
       BlueprintReadOnly,
       Category = "Monster|Director|Ambush",
       meta = (ClampMin = "1.0", Units = "s")
    )
    float AmbushDecisionCooldown = 20.0f;

    /**
     * 하나의 디렉터가 동시에 운용할 최대 매복 몬스터 수
     */
    UPROPERTY(
       EditAnywhere,
       BlueprintReadOnly,
       Category = "Monster|Director|Ambush",
       meta = (ClampMin = "0")
    )
    int32 MaxConcurrentAmbushers = 1;

    /**
     * 몬스터와 목표 플레이어 사이의 최소 매복 시작 거리
     *
     * 너무 가까운 몬스터는 숨으러 가지 않고 기존 전투를 유지합니다.
     */
    UPROPERTY(
       EditAnywhere,
       BlueprintReadOnly,
       Category = "Monster|Director|Ambush",
       meta = (ClampMin = "0.0", Units = "cm")
    )
    float MinimumAmbushDistance = 800.0f;
    
    // =========================================================================
    // 전술 우회·차단 설정
    // =========================================================================

    // 별도 전술 경로 없이 NavMesh에서 우회·차단 위치를 계산
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Director|Encirclement")
    bool bUseDynamicEncirclement = true;

    // 현재 이동 속도로 이 시간 뒤의 플레이어 위치를 예측
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Director|Encirclement",
        meta = (ClampMin = "0.5", ClampMax = "5.0", Units = "s"))
    float DynamicPredictionTime = 2.0f;

    // 직선 추적 경로에서 측면으로 떨어진 경유지의 거리
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Director|Encirclement",
        meta = (ClampMin = "250.0", Units = "cm"))
    float DynamicFlankOffset = 500.0f;

    // 오래 걸린 포위는 해제하고 새 상황에서 다시 판단
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Director|Encirclement",
        meta = (ClampMin = "6.0", Units = "s"))
    float DynamicPlanLifetime = 15.0f;

    // 동기 경로 탐색 비용을 제한할 몬스터 후보 수
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Director|Encirclement",
        meta = (ClampMin = "1", ClampMax = "8"))
    int32 MaximumDynamicCandidates = 4;

    // 레벨에 배치된 전술 경로 목록
    // 첫 스플라인 점은 우회 경유지,
    // 마지막 점은 최종 차단 위치
    UPROPERTY(
       EditInstanceOnly,
       BlueprintReadOnly,
       Category = "Monster|Director|Encirclement"
    )
    TArray<TObjectPtr<ABaruMonsterTacticalRoute>> TacticalRoutes;

    // 포위 행동을 시작하기 위해 필요한 최소 생존 몬스터 수
    UPROPERTY(
       EditAnywhere,
       BlueprintReadOnly,
       Category = "Monster|Director|Encirclement",
       meta = (ClampMin = "2")
    )
    int32 MinimumEncirclementParticipants = 2;

    // 플레이어가 차단 지점의 이 거리 안에 있을 때만
    // 해당 전술 경로를 사용
    UPROPERTY(
       EditAnywhere,
       BlueprintReadOnly,
       Category = "Monster|Director|Encirclement",
       meta = (ClampMin = "100.0", Units = "cm")
    )
    float EncirclementRouteActivationRadius = 1800.0f;

    // 너무 멀리 있는 몬스터가 포위 명령을 받아
    // 맵 전체를 횡단하지 않도록 제한
    UPROPERTY(
       EditAnywhere,
       BlueprintReadOnly,
       Category = "Monster|Director|Encirclement",
       meta = (ClampMin = "100.0", Units = "cm")
    )
    float MaximumEncirclementCandidateDistance = 3000.0f;

    // 실패한 경로 탐색을 매 배정 타이머마다 반복하지 않도록 제한
    UPROPERTY(
       EditAnywhere,
       BlueprintReadOnly,
       Category = "Monster|Director|Encirclement",
       meta = (ClampMin = "1.0", Units = "s")
    )
    float EncirclementDecisionCooldown = 5.0f;

private:
    // 지휘 대상의 수명을 유지하지 않도록 약한 참조로 보관
    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<ABaruMonsterCharacter>> RegisteredMonsters;
   
   // 디렉터 웨이브로 생성된 몬스터만 포위 후보로 사용
   UPROPERTY(Transient)
   TSet<TWeakObjectPtr<ABaruMonsterCharacter>> WaveSpawnedMonsters;

   // 이번 SpawnWave 호출에서 새로 등록된 개체
   UPROPERTY(Transient)
   TArray<TWeakObjectPtr<ABaruMonsterCharacter>> NewWaveSpawnedMonsters;

   // SpawnWave 실행 중 등록되는 새 몬스터를 구분
   bool bRegisteringWaveSpawn = false;
    
    // Key: 배정된 몬스터
    // Value: 해당 몬스터가 압박하도록 배정된 플레이어
    //
    // 약한 참조를 사용하므로 몬스터나 플레이어의 제거를 막지 않음
    UPROPERTY(Transient)
    TMap<
       TWeakObjectPtr<ABaruMonsterCharacter>,
       TWeakObjectPtr<APawn>
    > MonsterAssignments;

    // 파괴·사망한 몬스터와 유효하지 않은 플레이어의 배정 제거
    void RemoveInvalidAssignments();

    // 현재 위협도와 운영 상태를 기준으로 몬스터 배정 갱신
    void UpdateMonsterAssignments();

    // 해당 플레이어에게 보낼 수 있는 가장 가까운 미배정 몬스터 탐색
    ABaruMonsterCharacter* FindClosestUnassignedMonster(
       const APawn* PlayerPawn
    );

    // Actor Tick 대신 일정 간격으로 배정을 확인하는 타이머
    FTimerHandle MonsterAssignmentUpdateTimerHandle;
    
    // 현재 디렉터 상태에 맞는 웨이브 수량 반환
    int32 GetWaveSizeForCurrentState() const;
    
    /**
    * 현재 접속 중이며 살아 있는 실제 플레이어 수 반환
    *
    * AI 동료나 관전자는 스폰 배수에 포함하지 않는다.
    */
    int32 GetAlivePlayerCount() const;

    // 사용 가능한 스포너를 찾아 새 웨이브 생성 시도
    bool TrySpawnDirectorWave(APawn* TargetPlayer);

    // 마지막으로 웨이브 생성에 성공한 서버 시간
    // -1이면 아직 한 번도 생성하지 않은 상태
    double LastWaveSpawnTime = -1.0;

    // 파괴되거나 사망한 몬스터를 목록에서 제거
    void RemoveInvalidMonsters();

    // 등록 여부와 생존 상태를 확인하고 Controller 반환
    ABaruMonsterAIController* FindCommandController(
       ABaruMonsterCharacter* Monster
    );
    
    // 플레이어 캐릭터별 디렉터 위협도
    // Key: 어느 플레이어인가
    // Value: 그 플레이어의 위협도 점수
    // 약한 참조이므로 이 목록이 플레이어의 제거를 막지 않음
    // 현재는 Pawn 기준이라 캐릭터가 교체되면 위협도도 새로 시작
    UPROPERTY(Transient)
    TMap<TWeakObjectPtr<APawn>, float> PlayerThreatScores;

    // 파괴됐거나 조종이 해제된 플레이어의 기록을 제거
    void RemoveInvalidPlayerThreats();
    
    // 경과 시간에 따라 모든 플레이어의 디렉터 위협도 감소
    // 0까지 감소한 기록은 목록에서 제거
    void DecayPlayerThreats(float DeltaSeconds);
    
    // 플레이어 팀 전체가 현재 얼마나 벅찬지를 나타내는 값
    // 0에 가까움:
    // 체력이 충분하고 다운된 인원이 없으며 전투를 잘 처리 중
    // 100에 가까움:
    // 체력이 부족하거나 다운된 인원이 있고
    // 많은 몬스터에게 동시에 공격받는 상태
    // 이 값이 낮으면 Pressure 진입 후보가 되고
    // 높으면 Relief 진입 후보가 됨
    UPROPERTY(
       VisibleInstanceOnly,
       BlueprintReadOnly,
       Transient,
       Category = "Monster|Director|Team Burden",
       meta = (AllowPrivateAccess = "true")
    )
    float TeamBurden = 0.0f;
    
    // 현재 적용 중인 디렉터 운영 상태
    // 게임 시작 시에는 평상시로 시작
    //
    // 상태 변경은 SetDirectorState를 통해서만 처리하여
    // 이후 상태별 설정 적용도 한곳에서 관리
    UPROPERTY(
       VisibleInstanceOnly,
       BlueprintReadOnly,
       Transient,
       Category = "Monster|Director|State",
       meta = (AllowPrivateAccess = "true")
    )
    EBaruDirectorState CurrentDirectorState =
       EBaruDirectorState::Normal;
    
    // 엘리베이터가 보내는 탈출 저지 요청
    // 서버 StateTree의 Extraction 전환 조건으로 사용
    UPROPERTY(
       VisibleInstanceOnly,
       BlueprintReadOnly,
       Transient,
       Category = "Monster|Director|Extraction",
       meta = (AllowPrivateAccess = "true")
    )
    bool bExtractionActive = false;
    
    // 현재 PlayerState들을 읽어 팀 부담도를 다시 계산
    void RecalculateTeamBurden();

    // Actor Tick 대신 1초마다 부담도를 계산하기 위한 타이머
    FTimerHandle TeamBurdenUpdateTimerHandle;
    
    /**
    * 탈출 카운트다운 중 엘리베이터 밖에 남아 있는 플레이어
    *
    * 약한 참조이므로 플레이어 Pawn의 제거를 방해하지 않습니다.
    * 서버의 탈출 저지 웨이브 목표로만 사용합니다.
    */
    UPROPERTY(Transient)
    TWeakObjectPtr<APawn> ExtractionTargetPlayer;
    
    // 목표 플레이어를 매복할 수 있는 몬스터를 찾아 명령 전달
    // 현재는 자동 갱신에 연결하지 않고 준비만 함
    bool TryAssignAmbush(APawn* TargetPlayer);

    // 마지막으로 매복 배정을 판단한 서버 시간
    // -1이면 아직 한 번도 판단하지 않은 상태
    double LastAmbushDecisionTime = -1.0;
    
    // 현재 상황에서 측면 우회·차단 담당 몬스터를 선정
    bool TryAssignEncirclement(APawn* TargetPlayer);

    // 사망, 목표 이탈, 경로 이탈 등으로 끝난 포위 배정 정리
    void RefreshEncirclementAssignment();

    // 현재 퇴로 차단 담당 몬스터
    UPROPERTY(Transient)
    TWeakObjectPtr<ABaruMonsterCharacter>
       CurrentEncirclementBlocker;

    // 현재 차단 대상 플레이어
    UPROPERTY(Transient)
    TWeakObjectPtr<APawn>
       CurrentEncirclementTarget;

    // 현재 사용 중인 전술 경로
    UPROPERTY(Transient)
    TWeakObjectPtr<ABaruMonsterTacticalRoute>
       CurrentEncirclementRoute;

    // 마지막 포위 배정 판단 시간
    double LastEncirclementDecisionTime = -1.0;
    
    // 압박·탈출 저지 상태에서만 포위를 허용
    bool IsEncirclementAllowed() const;

    // 자동 포위 계획과 현재 명령의 유지 여부를 검사
    APawn* FindDynamicPressureTarget() const;
    bool TryAssignDynamicEncirclement(
        APawn* TargetPlayer,
        const TArray<TWeakObjectPtr<ABaruMonsterCharacter>>& NewWaveMonsters,
        int32 RequestedBlockers
    );
    void RefreshDynamicEncirclementAssignment();
    void FinishDynamicEncirclement(const TCHAR* Reason);
    void RefreshTacticalState();
    bool HasValidExtractionContext() const;

    FTimerHandle TacticalStateTimerHandle;

    UPROPERTY(Transient)
    TWeakObjectPtr<AActor> ExtractionSourceElevator;

    FVector ExtractionBoardingLocation = FVector::ZeroVector;
    double ExtractionDeadline = -1.0;
    double LastExtractionContextTime = -1.0;
    bool bHasExtractionContext = false;

   // 한 목표에게 동시에 배정할 차단 담당 수
   UPROPERTY(
       EditAnywhere,
       Category = "Monster|Director|Encirclement",
       meta = (ClampMin = "1", ClampMax = "4")
   )
   int32 PressureBlockerCount = 2;

   UPROPERTY(
       EditAnywhere,
       Category = "Monster|Director|Encirclement",
       meta = (ClampMin = "1", ClampMax = "4")
   )
   int32 ExtractionBlockerCount = 3;

   // 선택한 이동 경로와 차단 위치 표시
   UPROPERTY(
       EditAnywhere,
       Category = "Monster|Director|Encirclement"
   )
   bool bDrawDynamicEncirclementDebug = false;

   // 몬스터 한 마리가 수행하는 차단 계획
   struct FDynamicBlockerAssignment
   {
      // 배정된 몬스터와 명령을 받은 Controller
      TWeakObjectPtr<ABaruMonsterCharacter> Monster;
      TWeakObjectPtr<ABaruMonsterAIController> Controller;

      // 이 몬스터가 차단해야 할 플레이어
      TWeakObjectPtr<APawn> Target;

      // 다른 명령으로 바뀌었는지 확인할 배정 당시 버전
      uint32 Revision = 0;

      // 최종 차단 위치와 예상 플레이어 이동 경로
      FVector Block = FVector::ZeroVector;
      TArray<FVector> PlayerPath;

      // 플레이어 경로 시작점부터 차단 위치까지의 누적 거리
      double BlockProgress = 0.0;

      // 계획 시작 시각과 경로 이탈을 처음 확인한 시각
      double StartedAt = 0.0;
      double OffRouteSince = -1.0;

      // 차단 위치 도착 로그를 한 번만 출력하기 위한 상태
      bool bReachedBlock = false;
      
      // 현재 포위 담당의 접근 방향
      int32 ApproachSide = 0;
      
   };

   // 동시에 진행 중인 몬스터별 차단 계획
   TArray<FDynamicBlockerAssignment> DynamicAssignments;

   // 진행 중인 동적 포위 배정이 있는지 표시
   bool bHasDynamicEncirclement = false;

   // 배정이나 정리 도중 같은 처리가 다시 실행되는 것을 방지
   bool bUpdatingDynamicGroup = false;

   // 다음 검사에서 먼저 살펴볼 방향
   int32 DynamicCandidateCursor = 0;
   

   // 배정 당시 상태와 현재 상태를 비교하여 계획 유지 여부 판단
   EBaruDirectorState DynamicAssignmentState =
       EBaruDirectorState::Normal;

   // 지정한 몬스터의 배정만 종료하고 명령을 정리
   void FinishDynamicMember(
       int32 Index,
       const TCHAR* Reason
   );
   
   // 이 디렉터가 내린 집결 명령의 소유권 기록
   struct FExtractionRallyOrder
   {
      TWeakObjectPtr<ABaruMonsterCharacter> Monster;
      TWeakObjectPtr<ABaruMonsterAIController> Controller;
      uint32 Revision = 0;
   };

   TArray<FExtractionRallyOrder> ExtractionRallyOrders;

   // 명령 변경 중 중복 갱신 방지
   bool bRefreshingExtractionRally = false;

   // 전체 집결 배정과 종료 처리
   void RefreshExtractionRally();
    
};
