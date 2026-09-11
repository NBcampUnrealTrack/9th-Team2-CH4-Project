

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaruMonsterDirector.generated.h"

class ABaruMonsterCharacter;
class ABaruMonsterAIController;

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

protected:
	
	virtual void EndPlay(
	   const EEndPlayReason::Type EndPlayReason
   ) override;

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
	
};
