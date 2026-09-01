

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

protected:
	
	//몬스터가 주변 Actor를 감지할 때 사용하는 감각 기관
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|AI")
	TObjectPtr<UAIPerceptionComponent> MonsterPerceptionComponent;
	
	//시야 거리와 시야각처럼 시각 감지에 필요한 규칙을 보관
	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;
	
	// 몬스터의 이동 요청과 이동 설정을 관리
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|AI")
	TObjectPtr<UBaruMonsterNavigationComponent> MonsterNavigationComponent;
	
	//AIController가 몬스터의 몸을 조종하기 시작할 때 호출
	//조종할 몬스터가 가진 DataAsset을 읽고 시야 거리와 시야각을 설정
	virtual void OnPossess(APawn* InPawn) override;
	virtual void BeginPlay() override;
	
	// 감각 기관이 새로운 대상을 발견하거나 놓치면 호출
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
			
private:
	
	//DataAsset에 저장된 시각 설정값을 AIPerception의 SightConfig에 적용
	void ApplySightSettings(const UBaruMonsterDataAsset& MonsterDataAsset);
	// 조종 중인 몬스터의 DataAsset 설정을 초기화
	void InitializeFromControlledMonster();
	
	// 현재 몬스터의 시야 안에 있는 플레이어 목록
	// 몬스터는 플레이어를 소유하지 않으므로 약한 참조를 사용
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<APawn>> VisiblePlayerCandidates;
	
	// 현재 몬스터가 실제 추적 대상으로 선택한 플레이어
	// 플레이어를 소유하지 않으므로 약한 참조를 사용
	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> CurrentTarget;

	// 플레이어를 시야 후보 목록에 추가
	void AddVisiblePlayerCandidate(APawn* PlayerPawn);

	// 플레이어를 시야 후보 목록에서 제거
	void RemoveVisiblePlayerCandidate(APawn* PlayerPawn);

	// 파괴되었거나 월드에서 사라져 무효가 된 플레이어를 정리
	void RemoveInvalidPlayerCandidates();
	
	// 현재 보이는 플레이어 중 가장 가까운 대상을 선택
	void SelectClosestVisiblePlayer();
	
	// 플레이어를 놓친 위치를 저장하고 기억시간 타이머를 시작
	void RememberLastKnownTargetLocation(const FVector& TargetLocation);

	// 기억시간이 끝나거나 플레이어를 다시 발견하면 기억을 제거
	void ClearLastKnownTargetLocation();
	
	// 현재 추적 대상을 놓치기 직전에 마지막으로 확인한 위치
	UPROPERTY(Transient)
	FVector LastKnownTargetLocation = FVector::ZeroVector;

	// 유효한 마지막 목격 위치가 저장돼 있는지 나타냄
	// ZeroVector도 실제 위치일 수 있으므로 별도의 bool로 구분
	UPROPERTY(Transient)
	bool bHasLastKnownTargetLocation = false;

	// DataAsset에서 가져온 시야 기억시간을 실행 중 보관
	float SightMemoryDuration = 0.0f;

	// 기억시간이 끝났을 때 마지막 목격 정보를 제거할 타이머
	FTimerHandle SightMemoryTimerHandle;
	
	//=============
	//이동관련
	//=============
		
	// 현재 타깃과 마지막 목격 위치를 기준으로 이동 행동을 갱신
	void UpdateMovementFromPerceptionState();
	
	//=============
	//BT관련
	//=============
	
	// 현재 감지 상태를 Behavior Tree가 사용할 블랙보드에 반영
	void UpdateBlackboardFromPerceptionState();
	
};
