#include "Gimmicks/BaruControlRoomSpawner.h"
#include "Monster/Characters/BaruMonsterCharacter.h"
#include "Monster/AI/BaruMonsterAIController.h"
#include "Monster/Components/BaruMonsterNavigationComponent.h"
#include "Core/BaruGameMode.h"
#include "Interfaces/CombatInterface.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "BaruLog.h"

ABaruControlRoomSpawner::ABaruControlRoomSpawner()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    SetRootComponent(RootScene);

    // 기본 스폰 지점 1개 등록
    SpawnOffsets.Add(FVector::ZeroVector);
}

void ABaruControlRoomSpawner::StartSpawning()
{
    if (!HasAuthority() || (bTriggerOnce && bHasSpawned))
    {
        return;
    }

    if (!MonsterClass)
    {
        BARU_NET_LOG(this, LogBaruAI, Warning, TEXT("ControlRoomSpawner: MonsterClass is not assigned!"));
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    bHasSpawned = true;

    // 타깃이 될 살아있는 플레이어 탐색
    APawn* TargetPlayer = FindLivingTargetPlayer();
    const FVector TargetLocation = TargetPlayer ? TargetPlayer->GetActorLocation() : FVector::ZeroVector;

    ABaruGameMode* GameMode = World->GetAuthGameMode<ABaruGameMode>();
    const FTransform BaseTransform = GetActorTransform();

    // 예상 스폰 개수를 게임모드에 선등록 (스포너 방어 로직 연계)
    if (GameMode)
    {
        GameMode->RegisterExpectedSpawns(SpawnOffsets.Num());
    }

    for (const FVector& Offset : SpawnOffsets)
    {
        // 뷰포트 오프셋 좌표를 월드 좌표로 변환
        const FVector SpawnWorldLocation = BaseTransform.TransformPosition(Offset);
        const FRotator SpawnWorldRotation = GetActorRotation();

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        // 1. 몬스터 스폰
        ABaruMonsterCharacter* SpawnedMonster = World->SpawnActor<ABaruMonsterCharacter>(
            MonsterClass, SpawnWorldLocation, SpawnWorldRotation, SpawnParams);

        if (!SpawnedMonster)
        {
            continue;
        }

        // 2. 게임모드 생존 몬스터 목록에 등록
        if (GameMode)
        {
            GameMode->RegisterMonster(SpawnedMonster);
        }

        // 3. AI 돌진 및 타깃 주입
        if (bRushToPlayer && TargetPlayer)
        {
            if (ABaruMonsterAIController* MonsterAI = Cast<ABaruMonsterAIController>(SpawnedMonster->GetController()))
            {
                // (1) 플레이어에 대한 피해 위협도를 극대화하여 1순위 어그로 대상으로 지정
                MonsterAI->RegisterDamageThreat(TargetPlayer, 100.0f);

                // (2) 디렉터 조사 명령 주입 (Blackboard의 DirectorTargetLocation으로 이동)
                MonsterAI->ReceiveDirectorInvestigateCommand(TargetLocation);

                // (3) 내비게이션 컴포넌트를 최고 추적 속도(ChaseSpeed)로 올리고 이동 지시
                if (UBaruMonsterNavigationComponent* NavComp = MonsterAI->FindComponentByClass<UBaruMonsterNavigationComponent>())
                {
                    NavComp->SetControlledMonsterMoveSpeed(NavComp->GetChaseSpeed());
                    NavComp->MoveToLocation(TargetLocation);
                }
            }
        }
    }

    BARU_NET_LOG(this, LogBaruAI, Log, TEXT("ControlRoomSpawner: Successfully spawned %d monsters rushing to player."), SpawnOffsets.Num());
}

APawn* ABaruControlRoomSpawner::FindLivingTargetPlayer() const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        if (APlayerController* PC = It->Get())
        {
            if (APawn* Pawn = PC->GetPawn())
            {
                if (Pawn->Implements<UCombatInterface>())
                {
                    if (!ICombatInterface::Execute_IsDead(Pawn))
                    {
                        return Pawn;
                    }
                }
            }
        }
    }

    return nullptr;
}