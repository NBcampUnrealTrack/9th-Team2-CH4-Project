

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaruControlRoomSpawner.generated.h"

class ABaruMonsterCharacter;
class APawn;

/**
 * 몬스터 웨이브 스포너
 *
 * 기존 컨트롤룸 이벤트 스폰 기능을 유지하면서
 * 디렉터가 필요할 때 여러 마리를 웨이브 단위로
 * 생성할 수 있도록 확장한 스포너입니다.
 */
UCLASS()
class BARUGAME_API ABaruControlRoomSpawner : public AActor
{
    GENERATED_BODY()

public:
    ABaruControlRoomSpawner();

    /**
     * 기본 웨이브를 시작합니다.
     * BaruSpawnerControlVolume의 ResumeSpawning 동작에서도 호출됩니다.
     */
    UFUNCTION(
        BlueprintCallable,
        BlueprintAuthorityOnly,
        Category = "BARU|Spawner"
    )
    void StartSpawning();

    /**
     * 이후의 웨이브 생성을 정지합니다.
     * 이미 생성된 몬스터는 제거하지 않습니다.
     */
    UFUNCTION(
        BlueprintCallable,
        BlueprintAuthorityOnly,
        Category = "BARU|Spawner"
    )
    void StopSpawning();

    /**
     * 지정한 수만큼 몬스터 웨이브를 생성합니다.
     * 실제 생성에 성공한 몬스터 수를 반환합니다.
     */
    UFUNCTION(
        BlueprintCallable,
        BlueprintAuthorityOnly,
        Category = "BARU|Spawner"
    )
    int32 SpawnWave(
        int32 RequestedCount,
        APawn* TargetPlayer
    );

    /** 현재 새로운 웨이브를 생성할 수 있는 상태인지 반환합니다. */
    UFUNCTION(
        BlueprintPure,
        Category = "BARU|Spawner"
    )
    bool IsSpawningEnabled() const
    {
        return bSpawningEnabled;
    }

protected:
    UPROPERTY(
        VisibleAnywhere,
        BlueprintReadOnly,
        Category = "BARU|Components"
    )
    TObjectPtr<USceneComponent> RootScene;

    /** 스폰할 몬스터 클래스 */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "BARU|Spawner"
    )
    TSubclassOf<ABaruMonsterCharacter> MonsterClass;

    /**
     * 몬스터가 생성될 위치 목록입니다.
     * 각 위치는 스포너 액터를 기준으로 하는 상대 좌표입니다.
     */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "BARU|Spawner",
        meta = (MakeEditWidget = true)
    )
    TArray<FVector> SpawnOffsets;

    /** 기본 웨이브의 최소 생성 수 */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "BARU|Spawner|Wave",
        meta = (ClampMin = "1")
    )
    int32 MinWaveSize = 2;

    /** 기본 웨이브의 최대 생성 수 */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "BARU|Spawner|Wave",
        meta = (ClampMin = "1")
    )
    int32 MaxWaveSize = 4;

    /**
     * 이 스포너가 동시에 유지할 수 있는 최대 생존 몬스터 수입니다.
     * 0이면 제한하지 않습니다.
     */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "BARU|Spawner|Wave",
        meta = (ClampMin = "0")
    )
    int32 MaxAliveMonsters = 8;

    /** 한 번 성공한 뒤 다시 스폰하지 않을지 여부 */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "BARU|Spawner"
    )
    bool bTriggerOnce = true;

    /**
     * 생성 직후 플레이어의 위치를 조사하도록
     * Behavior Tree에 디렉터 명령을 전달할지 여부입니다.
     */
    UPROPERTY(
        EditAnywhere,
        BlueprintReadOnly,
        Category = "BARU|Spawner"
    )
    bool bRushToPlayer = true;

    /** 살아 있는 플레이어 한 명을 찾습니다. */
    APawn* FindLivingTargetPlayer() const;

    /** 지정된 위치에 몬스터 한 마리를 생성합니다. */
    ABaruMonsterCharacter* SpawnSingleMonster(
        const FVector& SpawnLocation,
        APawn* TargetPlayer
    );

    /** 제거됐거나 사망한 몬스터를 추적 목록에서 정리합니다. */
    void RemoveInvalidSpawnedMonsters();

private:
    /** 1회성 스포너가 이미 성공했는지 기록합니다. */
    bool bHasSpawned = false;

    /** 현재 새로운 웨이브 생성이 허용되어 있는지 나타냅니다. */
    bool bSpawningEnabled = true;

    /** 다음 웨이브에서 사용할 SpawnOffset 순번입니다. */
    int32 NextSpawnOffsetIndex = 0;

    /** 이 스포너가 생성한 생존 몬스터 목록입니다. */
    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<ABaruMonsterCharacter>> SpawnedMonsters;
};