

#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"
#include "BaruMonsterDirector.generated.h"

class ABaruMonsterCharacter;
class ABaruMonsterAIController;
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
 * 현재는 위협도에 따른 자동 배분이나 스폰 기능은 포함하지 않음
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

private:
	// 지휘 대상의 수명을 유지하지 않도록 약한 참조로 보관
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ABaruMonsterCharacter>> RegisteredMonsters;
	
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
	
};
