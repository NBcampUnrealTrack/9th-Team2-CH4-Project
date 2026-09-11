

#pragma once

#include "CoreMinimal.h"
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

protected:
	
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

private:
	// 지휘 대상의 수명을 유지하지 않도록 약한 참조로 보관
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ABaruMonsterCharacter>> RegisteredMonsters;

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
	
};
