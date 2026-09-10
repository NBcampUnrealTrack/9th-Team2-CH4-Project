#include "Gimmicks/BaruSpawnerControlVolume.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Character/BaruCharacter.h"
#include "BaruLog.h"

ABaruSpawnerControlVolume::ABaruSpawnerControlVolume()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
    SetRootComponent(TriggerVolume);
    TriggerVolume->SetBoxExtent(FVector(250.0f, 250.0f, 150.0f));
    TriggerVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    TriggerVolume->SetGenerateOverlapEvents(true);
}

void ABaruSpawnerControlVolume::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ABaruSpawnerControlVolume::HandleVolumeBeginOverlap);
    }
}

void ABaruSpawnerControlVolume::HandleVolumeBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!HasAuthority() || (bTriggerOnce && bHasTriggered))
    {
        return;
    }

    // 살아있는 플레이어 캐릭터인지 검증
    ABaruCharacter* PlayerChar = Cast<ABaruCharacter>(OtherActor);
    if (!PlayerChar || PlayerChar->IsDead_Implementation())
    {
        return;
    }

    bHasTriggered = true;
    ProcessSpawnerControl();
}

void ABaruSpawnerControlVolume::ProcessSpawnerControl()
{
    TArray<AActor*> CombinedSpawners = TargetSpawners;

    // 태그 기반 스포너 수집 병합
    if (!TargetSpawnerTag.IsNone())
    {
        TArray<AActor*> TaggedActors;
        UGameplayStatics::GetAllActorsWithTag(GetWorld(), TargetSpawnerTag, TaggedActors);
        for (AActor* Act : TaggedActors)
        {
            CombinedSpawners.AddUnique(Act);
        }
    }

    int32 AffectedCount = 0;

    for (AActor* SpawnerActor : CombinedSpawners)
    {
        if (!IsValid(SpawnerActor))
        {
            continue;
        }

        switch (ControlAction)
        {
        case EBaruSpawnerControlAction::StopSpawning:
            // 몬스터 스포너의 액터 틱 비활성화
            SpawnerActor->SetActorTickEnabled(false);
            // 스포너 블루프린트 또는 C++의 StopSpawning 함수 안전 호출
            SpawnerActor->CallFunctionByNameWithArguments(TEXT("StopSpawning"), *GLog, nullptr, true);
            AffectedCount++;
            break;

        case EBaruSpawnerControlAction::ResumeSpawning:
            SpawnerActor->SetActorTickEnabled(true);
            SpawnerActor->CallFunctionByNameWithArguments(TEXT("StartSpawning"), *GLog, nullptr, true);
            AffectedCount++;
            break;

        case EBaruSpawnerControlAction::DestroySpawners:
            SpawnerActor->Destroy();
            AffectedCount++;
            break;
        }
    }

    BARU_NET_LOG(this, LogBaruAI, Log, TEXT("SpawnerControlVolume triggered: Action=%d, Affected Spawners=%d"), 
        static_cast<int32>(ControlAction), AffectedCount);

    BP_OnSpawnersControlled(ControlAction, AffectedCount);

    if (bTriggerOnce)
    {
        TriggerVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}