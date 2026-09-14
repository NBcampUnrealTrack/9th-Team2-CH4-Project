#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaruControlRoomSpawner.generated.h"

class ABaruMonsterCharacter;

/**
 * 컨트롤 룸 전용 몬스터 스폰 기믹 액터
 * BaruSpawnerControlVolume에서 'ResumeSpawning' 신호를 받아 몬스터를 생성하고 플레이어에게 돌진시킵니다.
 */
UCLASS()
class BARUGAME_API ABaruControlRoomSpawner : public AActor
{
	GENERATED_BODY()

public:
	ABaruControlRoomSpawner();

	/** BaruSpawnerControlVolume의 ResumeSpawning 액션에 의해 리플렉션으로 호출되는 함수 */
	UFUNCTION(BlueprintCallable, Category = "BARU|Spawner")
	void StartSpawning();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
	TObjectPtr<USceneComponent> RootScene;

	/** 스폰할 몬스터 클래스 (BP_BaruMonster 등) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Spawner")
	TSubclassOf<ABaruMonsterCharacter> MonsterClass;

	/** 
	 * 컨트롤 룸 내 스폰 지점 오프셋 목록 
	 * 뷰포트에서 다이아몬드 위젯을 마우스로 끌어 원하는 스폰 위치를 배치할 수 있습니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Spawner", meta = (MakeEditWidget = true))
	TArray<FVector> SpawnOffsets;

	/** 1회 발동 후 중복 스폰 차단 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Spawner")
	bool bTriggerOnce = true;

	/** 스폰 즉시 추적(Chase) 속도로 질주 명령을 내릴지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Spawner")
	bool bRushToPlayer = true;

	/** 살아있는 유효한 플레이어를 탐색하는 헬퍼 함수 */
	APawn* FindLivingTargetPlayer() const;

private:
	bool bHasSpawned = false;
};