

#include "Gimmicks/BaruControlRoomSpawner.h"

#include "Monster/Characters/BaruMonsterCharacter.h"
#include "Monster/AI/BaruMonsterAIController.h"
#include "Core/BaruGameMode.h"
#include "Interfaces/CombatInterface.h"

#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "BaruLog.h"

ABaruControlRoomSpawner::ABaruControlRoomSpawner()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    RootScene = CreateDefaultSubobject<USceneComponent>(
        TEXT("RootScene")
    );

    SetRootComponent(RootScene);

    // 스폰 위치가 하나도 없는 초기 상태에서도
    // 액터 위치에서 몬스터를 생성할 수 있도록 기본 위치 등록
    SpawnOffsets.Add(FVector::ZeroVector);
}

void ABaruControlRoomSpawner::StartSpawning()
{
    if (!HasAuthority())
    {
        return;
    }

    // 제어 볼륨의 ResumeSpawning 동작으로 호출될 수 있으므로
    // 새로운 웨이브 생성을 다시 허용
    bSpawningEnabled = true;

    if (bTriggerOnce && bHasSpawned)
    {
        return;
    }

    // 에디터에서 최소값과 최대값을 반대로 입력해도
    // 안전하게 사용할 수 있도록 범위를 보정
    const int32 SafeMinWaveSize =
        FMath::Max(1, MinWaveSize);

    const int32 SafeMaxWaveSize =
        FMath::Max(SafeMinWaveSize, MaxWaveSize);

    const int32 RequestedCount = FMath::RandRange(
        SafeMinWaveSize,
        SafeMaxWaveSize
    );

    APawn* TargetPlayer = FindLivingTargetPlayer();

    SpawnWave(
        RequestedCount,
        TargetPlayer
    );
}

void ABaruControlRoomSpawner::StopSpawning()
{
    if (!HasAuthority())
    {
        return;
    }

    // 이미 생성된 몬스터는 유지하고
    // 이후에 들어오는 웨이브 요청만 차단
    bSpawningEnabled = false;

    BARU_NET_LOG(
        this,
        LogBaruAI,
        Log,
        TEXT("ControlRoomSpawner: Spawning stopped.")
    );
}

int32 ABaruControlRoomSpawner::SpawnWave(
    int32 RequestedCount,
    APawn* TargetPlayer
)
{
    if (!HasAuthority() ||
        !bSpawningEnabled ||
        RequestedCount <= 0)
    {
        return 0;
    }

    if (bTriggerOnce && bHasSpawned)
    {
        return 0;
    }

    if (!MonsterClass)
    {
        BARU_NET_LOG(
            this,
            LogBaruAI,
            Warning,
            TEXT(
                "ControlRoomSpawner: "
                "MonsterClass is not assigned."
            )
        );

        return 0;
    }

    if (SpawnOffsets.IsEmpty())
    {
        BARU_NET_LOG(
            this,
            LogBaruAI,
            Warning,
            TEXT(
                "ControlRoomSpawner: "
                "No SpawnOffsets are assigned."
            )
        );

        return 0;
    }

    // 이전 웨이브에서 죽었거나 제거된 몬스터를 정리
    RemoveInvalidSpawnedMonsters();

    int32 AllowedCount = RequestedCount;

    // 이 스포너가 유지할 수 있는 최대 생존 수 적용
    if (MaxAliveMonsters > 0)
    {
        const int32 RemainingCapacity =
            FMath::Max(
                0,
                MaxAliveMonsters - SpawnedMonsters.Num()
            );

        AllowedCount = FMath::Min(
            AllowedCount,
            RemainingCapacity
        );
    }

    // 한 웨이브에서 같은 위치를 중복 사용하지 않도록
    // 현재 등록된 스폰 위치 수까지만 허용
    AllowedCount = FMath::Min(
        AllowedCount,
        SpawnOffsets.Num()
    );

    if (AllowedCount <= 0)
    {
        BARU_NET_LOG(
            this,
            LogBaruAI,
            Log,
            TEXT(
                "ControlRoomSpawner: "
                "Wave blocked by alive monster limit."
            )
        );

        return 0;
    }

    // 타깃이 전달되지 않았다면 현재 살아 있는 플레이어 탐색
    if (!IsValid(TargetPlayer))
    {
        TargetPlayer = FindLivingTargetPlayer();
    }

    const FTransform BaseTransform = GetActorTransform();
    int32 SuccessfullySpawnedCount = 0;

    for (int32 Index = 0; Index < AllowedCount; ++Index)
    {
        // 웨이브마다 다음 위치부터 사용하도록 순환
        const int32 OffsetIndex =
            NextSpawnOffsetIndex % SpawnOffsets.Num();

        NextSpawnOffsetIndex =
            (NextSpawnOffsetIndex + 1) %
            SpawnOffsets.Num();

        // 스포너 기준 상대 좌표를 실제 월드 좌표로 변환
        const FVector SpawnWorldLocation =
            BaseTransform.TransformPosition(
                SpawnOffsets[OffsetIndex]
            );

        ABaruMonsterCharacter* SpawnedMonster =
            SpawnSingleMonster(
                SpawnWorldLocation,
                TargetPlayer
            );

        if (!IsValid(SpawnedMonster))
        {
            continue;
        }

        SpawnedMonsters.Add(SpawnedMonster);
        SuccessfullySpawnedCount++;
    }

    // 한 마리 이상 실제 생성된 경우에만
    // 1회성 스포너의 발동 완료를 기록
    if (SuccessfullySpawnedCount > 0)
    {
        bHasSpawned = true;
    }

    BARU_NET_LOG(
        this,
        LogBaruAI,
        Log,
        TEXT(
            "ControlRoomSpawner: "
            "Wave finished. Requested=%d, Spawned=%d, Alive=%d"
        ),
        RequestedCount,
        SuccessfullySpawnedCount,
        SpawnedMonsters.Num()
    );

    return SuccessfullySpawnedCount;
}

ABaruMonsterCharacter*
ABaruControlRoomSpawner::SpawnSingleMonster(
    const FVector& SpawnLocation,
    APawn* TargetPlayer
)
{
    UWorld* World = GetWorld();

    if (!IsValid(World) || !MonsterClass)
    {
        return nullptr;
    }

    const FTransform SpawnTransform(
        GetActorRotation(),
        SpawnLocation,
        FVector::OneVector
    );

    /*
     * Deferred Spawn을 사용합니다.
     *
     * 일반 SpawnActor는 몬스터 BeginPlay가 먼저 실행되어
     * GameMode 등록 시점을 정확히 맞추기 어렵습니다.
     *
     * Deferred Spawn은 액터를 먼저 준비한 다음
     * ExpectedSpawns를 등록하고 BeginPlay를 실행할 수 있습니다.
     */
    ABaruMonsterCharacter* SpawnedMonster =
        World->SpawnActorDeferred<ABaruMonsterCharacter>(
            MonsterClass,
            SpawnTransform,
            this,
            nullptr,
            ESpawnActorCollisionHandlingMethod::
                AdjustIfPossibleButAlwaysSpawn
        );

    if (!IsValid(SpawnedMonster))
    {
        return nullptr;
    }

    // 몬스터 BeginPlay에서 RegisterMonster가 호출되기 전에
    // 앞으로 생성될 몬스터 한 마리를 대기 수에 등록
    if (ABaruGameMode* GameMode =
        World->GetAuthGameMode<ABaruGameMode>())
    {
        GameMode->RegisterExpectedSpawns(1);
    }

    // 실제 생성 완료 및 BeginPlay 실행
    UGameplayStatics::FinishSpawningActor(
        SpawnedMonster,
        SpawnTransform
    );

    if (!IsValid(SpawnedMonster))
    {
        return nullptr;
    }

    // Auto Possess 설정이 누락된 블루프린트에도 대응
    if (!IsValid(SpawnedMonster->GetController()))
    {
        SpawnedMonster->SpawnDefaultController();
    }

    if (bRushToPlayer && IsValid(TargetPlayer))
    {
        if (ABaruMonsterAIController* MonsterAI =
            Cast<ABaruMonsterAIController>(
                SpawnedMonster->GetController()
            ))
        {
            /*
             * 직접 MoveToLocation을 호출하지 않습니다.
             * 이동은 Behavior Tree가 담당하고,
             * 여기서는 조사할 위치만 전달합니다.
             */
            MonsterAI->ReceiveDirectorInvestigateCommand(
                TargetPlayer->GetActorLocation()
            );
        }
    }

    return SpawnedMonster;
}

void ABaruControlRoomSpawner::
RemoveInvalidSpawnedMonsters()
{
    SpawnedMonsters.RemoveAll(
        [](const TWeakObjectPtr<
            ABaruMonsterCharacter>& Entry)
        {
            ABaruMonsterCharacter* Monster = Entry.Get();

            if (!IsValid(Monster))
            {
                return true;
            }

            // 죽은 액터가 즉시 제거되지 않더라도
            // 생존 몬스터 제한에서는 제외
            return Monster->Implements<UCombatInterface>() &&
                ICombatInterface::Execute_IsDead(Monster);
        }
    );
}

APawn* ABaruControlRoomSpawner::
FindLivingTargetPlayer() const
{
    UWorld* World = GetWorld();

    if (!IsValid(World))
    {
        return nullptr;
    }

    for (FConstPlayerControllerIterator Iterator =
            World->GetPlayerControllerIterator();
         Iterator;
         ++Iterator)
    {
        APlayerController* PlayerController =
            Iterator->Get();

        if (!IsValid(PlayerController))
        {
            continue;
        }

        APawn* PlayerPawn =
            PlayerController->GetPawn();

        if (!IsValid(PlayerPawn) ||
            !PlayerPawn->IsPlayerControlled())
        {
            continue;
        }

        if (PlayerPawn->Implements<UCombatInterface>() &&
            !ICombatInterface::Execute_IsDead(PlayerPawn))
        {
            return PlayerPawn;
        }
    }

    return nullptr;
}