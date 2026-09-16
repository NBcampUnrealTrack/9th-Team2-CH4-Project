

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TimerManager.h"
#include "Perception/AIPerceptionTypes.h"
#include "BaruMonsterAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UBaruMonsterDataAsset;
class UBaruMonsterNavigationComponent;
class ABaruMonsterTacticalRoute;

// 디렉터가 몬스터에게 내릴 수 있는 기본 명령
UENUM(BlueprintType)
enum class EBaruMonsterDirectorCommand : uint8
{
    // 디렉터 명령 없이 개별 AI 판단 사용
    None UMETA(DisplayName = "명령 없음"),

    // 지정 위치로 이동해서 조사
    Investigate UMETA(DisplayName = "이동 및 조사"),

    // 지정 플레이어를 대상으로 은폐 위치를 찾아 매복
    Ambush UMETA(DisplayName = "매복"),

    // 대기 명령. 실제 추적·수색 우선순위는 BT가 결정
    Hold UMETA(DisplayName = "현재 위치 대기"),

    // 다른 통로를 경유하여 목표의 퇴로를 차단
    Encircle UMETA(DisplayName = "우회 및 차단"),
	
   // 탈출 중인 플레이어를 향해 집결
    ExtractionRally UMETA(DisplayName = "탈출 저지 집결")
};

UCLASS()
class BARUGAME_API ABaruMonsterAIController : public AAIController
{
    GENERATED_BODY()

public:
    ABaruMonsterAIController();

    // 현재 추적 대상으로 선택된 플레이어를 반환
    UFUNCTION(BlueprintPure, Category = "Monster|AI")
    APawn* GetCurrentTarget() const;

    // 기억 중인 마지막 목격 위치가 있는지 반환
    UFUNCTION(BlueprintPure, Category = "Monster|AI")
    bool HasLastKnownTargetLocation() const;

    // 마지막으로 목격한 플레이어의 위치를 반환
    UFUNCTION(BlueprintPure, Category = "Monster|AI")
    FVector GetLastKnownTargetLocation() const;

    // 마지막 목격 위치 주변 수색이 끝났을 때 호출
    // 저장된 위치와 관련 타이머 및 Blackboard 값을 정리
    UFUNCTION(
       BlueprintCallable,
       BlueprintAuthorityOnly,
       Category = "Monster|AI|Perception"
    )
    void CompleteLastKnownTargetSearch();
	
   // 탈출 중인 플레이어를 향해 집결 명령
   bool ReceiveDirectorExtractionRallyCommand(
       APawn* TargetPlayer
   );

   // 현재 집결 대상 확인
   APawn* GetExtractionRallyTarget() const
   {
      return ExtractionRallyTarget.Get();
   }

protected:
    // 몬스터가 주변 Actor를 감지할 때 사용하는 감각 기관
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|AI")
    TObjectPtr<UAIPerceptionComponent> MonsterPerceptionComponent;

    // 시야 거리와 시야각처럼 시각 감지에 필요한 규칙을 보관
    UPROPERTY()
    TObjectPtr<UAISenseConfig_Sight> SightConfig;

    // 몬스터의 이동 요청과 이동 설정을 관리
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|AI")
    TObjectPtr<UBaruMonsterNavigationComponent> MonsterNavigationComponent;

    // AIController가 몬스터의 몸을 조종하기 시작할 때 호출
    // 몬스터의 DataAsset을 읽고 시야 거리와 시야각을 설정
    virtual void OnPossess(APawn* InPawn) override;
    virtual void BeginPlay() override;

    // 감각 기관이 새로운 대상을 발견하거나 놓치면 호출
    UFUNCTION()
    void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

    // 조종이 해제되면 기존 몬스터의 타이머와 대상 정보를 정리
    virtual void OnUnPossess() override;

    // Controller가 제거될 때 타이머와 감지 연결 정리
    virtual void EndPlay(
       const EEndPlayReason::Type EndPlayReason
    ) override;

private:
    // DataAsset에 저장된 시각 설정값을 SightConfig에 적용
    void ApplySightSettings(const UBaruMonsterDataAsset& MonsterDataAsset);

    // 조종 중인 몬스터의 DataAsset 설정을 초기화
    void InitializeFromControlledMonster();

    // 현재 몬스터의 시야 안에 있는 플레이어 목록
    // 플레이어를 소유하지 않으므로 약한 참조를 사용
    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<APawn>> VisiblePlayerCandidates;

    // 현재 몬스터가 실제 추적 대상으로 선택한 플레이어
    UPROPERTY(Transient)
    TWeakObjectPtr<APawn> CurrentTarget;

    // 플레이어를 시야 후보 목록에 추가
    void AddVisiblePlayerCandidate(APawn* PlayerPawn);

    // 플레이어를 시야 후보 목록에서 제거
    void RemoveVisiblePlayerCandidate(APawn* PlayerPawn);

    // 파괴되었거나 무효가 된 플레이어를 정리
    void RemoveInvalidPlayerCandidates();

    // 현재 보이는 유효한 플레이어 중 위협도 점수가 가장 높은 대상 선택
    void SelectHighestThreatVisiblePlayer();

    // 플레이어를 놓친 위치를 저장하고 기억시간 타이머를 시작
    void RememberLastKnownTargetLocation(const FVector& TargetLocation);

    // 기억시간이 끝나거나 플레이어를 다시 발견하면 기억을 제거
    void ClearLastKnownTargetLocation();

    // 현재 추적 대상을 놓치기 직전에 마지막으로 확인한 위치
    UPROPERTY(Transient)
    FVector LastKnownTargetLocation = FVector::ZeroVector;

    // 원점도 실제 위치일 수 있으므로 별도의 bool로 유효 여부를 구분
    UPROPERTY(Transient)
    bool bHasLastKnownTargetLocation = false;

    // DataAsset에서 가져온 시야 기억시간을 실행 중 보관
    float SightMemoryDuration = 0.0f;

    // 기억시간이 끝났을 때 마지막 목격 정보를 제거할 타이머
    FTimerHandle SightMemoryTimerHandle;
   
   // 시야에 보이지 않아도 집결 대상으로 유지
   UPROPERTY(Transient)
   TWeakObjectPtr<APawn> ExtractionRallyTarget;

    // =========================================================================
    // 근접 전투 대상 유지
    // =========================================================================

    // 이 거리 안이고 건물 벽이 없다면 시야가 잠시 끊겨도 추적 유지
    float CombatTargetRetentionDistance = 0.0f;

    // 거리와 벽을 다시 확인하는 간격
    float CombatTargetRetentionCheckInterval = 0.25f;

    // 시야 손실 판정을 보류하고 있는 플레이어
    UPROPERTY(Transient)
    TWeakObjectPtr<APawn> PendingLostCombatTarget;

    // 실제로 마지막까지 확인할 수 있었던 플레이어 위치
    UPROPERTY(Transient)
    FVector PendingLostCombatTargetLocation = FVector::ZeroVector;

    // 보류 중인 전투 대상 상태를 다시 검사하는 타이머
    FTimerHandle CombatTargetRetentionTimerHandle;

    // 가까운 전투 대상을 계속 유지할 수 있는지 검사
    // 거리 안에 있고 WorldStatic 벽이 없을 때 true
    bool CanRetainCombatTarget(APawn* TargetPawn) const;

    // 시야 손실을 바로 확정하지 않고 전투 대상 유지 검사 시작
    void BeginCombatTargetRetention(
       APawn* TargetPawn,
       const FVector& LastVisibleLocation
    );

    // 일정 간격마다 거리와 벽 상태를 다시 검사
    void ReevaluateCombatTargetRetention();

    // 시야 손실을 최종 확정하고 마지막 위치 수색으로 전환
    void ConfirmPlayerLost(
       APawn* LostPlayer,
       const FVector& LastVisibleLocation
    );

    // 플레이어를 다시 발견하거나 AI가 종료될 때 임시 정보 정리
    void CancelCombatTargetRetention(APawn* TargetPawn = nullptr);

    // 현재 감지 상태를 Behavior Tree가 사용할 Blackboard에 반영
    void UpdateBlackboardFromPerceptionState();

public:
    // 몬스터가 실제로 받은 피해를 공격자의 위협도로 등록
    // 그로기 중에도 위협도는 누적하며, 서버에서만 처리
    void RegisterDamageThreat(APawn* AttackerPawn, float DamageAmount);

private:
    // 플레이어별 누적 피해 위협도
    TMap<TWeakObjectPtr<APawn>, float> DamageThreatByPlayer;

    // 위협도 감소와 대상 재선택을 주기적으로 실행
    FTimerHandle ThreatUpdateTimerHandle;

    // 실제 경과시간을 기준으로 위협도를 감소시키는 데 사용
    double LastThreatUpdateTime = 0.0;

    // DataAsset에서 갱신 간격을 읽고 타이머 시작
    void StartThreatUpdates();

    // 누적 위협도를 감소시키고 현재 대상을 다시 선택
    void UpdateThreat();

    // 거리·피해 위협도·현재 대상 유지 보정을 합산
    float CalculateThreatScore(APawn* CandidatePawn) const;

public:
    // =========================================================================
    // 디렉터 명령
    // =========================================================================

    // 지정 플레이어를 대상으로 매복 준비 명령을 받음
    UFUNCTION(
       BlueprintCallable,
       BlueprintAuthorityOnly,
       Category = "Monster|AI|Director"
    )
    bool ReceiveDirectorAmbushCommand(APawn* TargetPlayer);

    // 배치된 전술 경로를 이용해 우회한 뒤 목표의 퇴로를 차단
    UFUNCTION(
       BlueprintCallable,
       BlueprintAuthorityOnly,
       Category = "Monster|AI|Director"
    )
    bool ReceiveDirectorEncirclementCommand(
       APawn* TargetPlayer,
       ABaruMonsterTacticalRoute* TacticalRoute
    );

    // TacticalRoute 없이 Director가 검증한 우회·차단 좌표를 전달받음
    // 재배정할 때는 기존 포위 명령을 먼저 해제해야 함
    bool ReceiveDirectorDynamicEncirclementCommand(
       APawn* TargetPlayer,
       const FVector& RouteLocation,
       const FVector& BlockLocation
    );

    // Director가 자신이 배정한 동적 포위인지 확인할 때 사용
    bool IsUsingDynamicEncirclement() const
    {
       return bUsesDynamicEncirclement;
    }

    // 일반 시야 추적 대상과 별도로 유지하는 포위 대상
    APawn* GetEncirclementTarget() const
    {
       return EncirclementTarget.Get();
    }

    // 포위 BT가 이동과 대기를 마치면 Hold로 전환
    // 이후 추적·수색 우선순위는 기존 BT 설정을 따름
    UFUNCTION(
       BlueprintCallable,
       BlueprintAuthorityOnly,
       Category = "Monster|AI|Director"
    )
    void CompleteDirectorEncirclementCommand();

    // 지정 위치로 이동·조사하도록 명령
    // true는 접수 성공이며 경로·도착 성공은 별도 확인
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Monster|AI|Director")
    bool ReceiveDirectorInvestigateCommand(const FVector& TargetLocation);

    // 대기 명령. 추적·수색이 진행 중이면 해당 행동을 우선
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Monster|AI|Director")
    void ReceiveDirectorHoldCommand();

    // 디렉터 명령을 해제하고 개별 AI 판단으로 복귀
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Monster|AI|Director")
    void ClearDirectorCommand();

    // 현재 접수된 명령을 확인
    UFUNCTION(BlueprintPure, Category = "Monster|AI|Director")
    EBaruMonsterDirectorCommand GetDirectorCommand() const
    {
       return DirectorCommand;
    }

    // 현재 명령의 버전 번호
    uint32 GetDirectorCommandRevision() const
    {
       return DirectorCommandRevision;
    }

private:
    // =========================================================================
    // 디렉터 명령 상태
    // =========================================================================

    // AI 판단은 서버에서 하므로 명령 상태는 복제하지 않음
    UPROPERTY(Transient)
    EBaruMonsterDirectorCommand DirectorCommand = EBaruMonsterDirectorCommand::None;

    // 이동·조사 명령의 목적지. 원점도 목적지로 사용 가능
    UPROPERTY(Transient)
    FVector DirectorTargetLocation = FVector::ZeroVector;

    // 시야가 끊겨도 매복이 끝날 때까지 기억할 목표 플레이어
    UPROPERTY(Transient)
    TWeakObjectPtr<APawn> AmbushTarget;

    // 포위·차단 명령의 대상 플레이어
    UPROPERTY(Transient)
    TWeakObjectPtr<APawn> EncirclementTarget;

    // 현재 사용 중인 전술 경로. 명령 해제 시 예약을 반환
    UPROPERTY(Transient)
    TWeakObjectPtr<ABaruMonsterTacticalRoute> ActiveTacticalRoute;

    // 먼저 이동할 측면 통로 경유지
    UPROPERTY(Transient)
    FVector EncirclementRouteLocation = FVector::ZeroVector;

    // 최종적으로 퇴로를 막을 위치
    UPROPERTY(Transient)
    FVector EncirclementBlockLocation = FVector::ZeroVector;

    // 배치 경로 대신 Director가 계산한 좌표를 사용하는지 구분
    UPROPERTY(Transient)
    bool bUsesDynamicEncirclement = false;

    // 전술 경로 예약과 고정/동적 포위 데이터를 함께 정리
    void ReleaseActiveTacticalRoute();

    // 현재 명령을 Behavior Tree의 Blackboard에 반영
    void UpdateBlackboardFromDirectorState();

    // 명령이 변경될 때마다 증가
    uint32 DirectorCommandRevision = 0;
};
