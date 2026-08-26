

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "BaruMonsterAIController.generated.h"


class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UBaruMonsterDataAsset;

UCLASS()
class BARUGAME_API ABaruMonsterAIController : public AAIController
{
	GENERATED_BODY()

public:
	
	ABaruMonsterAIController();

protected:
	
	//몬스터가 주변 Actor를 감지할 때 사용하는 감각 기관
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|AI")
	TObjectPtr<UAIPerceptionComponent> MonsterPerceptionComponent;
	
	//시야 거리와 시야각처럼 시각 감지에 필요한 규칙을 보관
	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;
	
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
	// AI 감각 기관이 준비된 다음 몬스터의 시야 설정을 적용
	void InitializeSightFromControlledMonster();
	
};
