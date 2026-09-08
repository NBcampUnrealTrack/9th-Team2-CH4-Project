// BaruMapTransitionVolume.cpp
#include "Gimmicks/BaruMapTransitionVolume.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Character/BaruCharacter.h"
#include "Player/BaruPlayerState.h"
#include "Core/BaruGameMode.h"
#include "Core/BaruLobbyGameMode.h"
#include "BaruLog.h"

ABaruMapTransitionVolume::ABaruMapTransitionVolume()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    TransitionTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("TransitionTrigger"));
    SetRootComponent(TransitionTrigger);
    TransitionTrigger->SetBoxExtent(FVector(200.0f, 200.0f, 100.0f));
    TransitionTrigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    TransitionTrigger->SetGenerateOverlapEvents(true);
}

void ABaruMapTransitionVolume::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        TransitionTrigger->OnComponentBeginOverlap.AddDynamic(this, &ABaruMapTransitionVolume::HandleVolumeBeginOverlap);
        // [중요 해결] 이탈 감지 바인딩 추가
        TransitionTrigger->OnComponentEndOverlap.AddDynamic(this, &ABaruMapTransitionVolume::HandleVolumeEndOverlap);
    }
}

void ABaruMapTransitionVolume::HandleVolumeBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!HasAuthority() || bHasTriggered || !IsValid(OtherActor)) return;

    ABaruCharacter* PlayerCharacter = Cast<ABaruCharacter>(OtherActor);
    if (!PlayerCharacter || PlayerCharacter->IsDead_Implementation()) return;

    OverlappedPlayers.Add(PlayerCharacter);
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("TransitionVolume: Entered by %s (Current: %d)"), *PlayerCharacter->GetName(), OverlappedPlayers.Num());

    if (bRequireAllAlivePlayers)
    {
        if (!CheckAllPlayersInVolume())
        {
            return;
        }
    }

    bHasTriggered = true;
    BP_OnTransitionTriggered();

    if (TransitionDelay > 0.0f)
    {
        GetWorldTimerManager().SetTimer(
            TransitionTimerHandle,
            this,
            &ABaruMapTransitionVolume::ExecuteTransition,
            TransitionDelay,
            false
        );
    }
    else
    {
        ExecuteTransition();
    }
}

void ABaruMapTransitionVolume::HandleVolumeEndOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    if (!HasAuthority() || !IsValid(OtherActor)) return;

    ABaruCharacter* PlayerCharacter = Cast<ABaruCharacter>(OtherActor);
    if (!PlayerCharacter) return;

    OverlappedPlayers.Remove(PlayerCharacter);
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("TransitionVolume: Exited by %s (Current: %d)"), *PlayerCharacter->GetName(), OverlappedPlayers.Num());

    // 전원 진입 조건인데 지연 대기 중 플레이어가 이탈한 경우 이동 취소
    if (bRequireAllAlivePlayers && bHasTriggered && !CheckAllPlayersInVolume())
    {
        bHasTriggered = false;
        GetWorldTimerManager().ClearTimer(TransitionTimerHandle);
        BP_OnTransitionCancelled();
        BARU_NET_LOG(this, LogBaruSession, Log, TEXT("TransitionVolume: Player left, transition cancelled."));
    }
}

bool ABaruMapTransitionVolume::CheckAllPlayersInVolume() const
{
    int32 RequiredCount = 0;
    int32 CurrentCount = 0;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (IsValid(PC) && !PC->IsPendingKillPending())
        {
            if (const ABaruPlayerState* PS = PC->GetPlayerState<ABaruPlayerState>())
            {
                if (PS->IsAlive())
                {
                    RequiredCount++;
                    if (PC->GetPawn() && OverlappedPlayers.Contains(PC->GetPawn()))
                    {
                        CurrentCount++;
                    }
                }
            }
        }
    }

    return (RequiredCount > 0) && (CurrentCount >= RequiredCount);
}

void ABaruMapTransitionVolume::ExecuteTransition()
{
    if (TargetLevel.IsNull())
    {
        BARU_NET_LOG(this, LogBaruSession, Error, TEXT("TransitionVolume: TargetLevel is NULL!"));
        return;
    }

    FString TargetMapURL = TargetLevel.ToSoftObjectPath().GetLongPackageName();
    if (TargetMapURL.IsEmpty())
    {
        TargetMapURL = TargetLevel.GetAssetName();
    }

    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("TransitionVolume: Transitioning to '%s'"), *TargetMapURL);

    AGameModeBase* AuthGM = GetWorld()->GetAuthGameMode();
    if (!AuthGM) return;

    if (ABaruLobbyGameMode* LobbyGM = Cast<ABaruLobbyGameMode>(AuthGM))
    {
        LobbyGM->StartGameRaid(TargetMapURL);
        return;
    }

    if (ABaruGameMode* IngameGM = Cast<ABaruGameMode>(AuthGM))
    {
        IngameGM->RequestLevelTransition(TargetMapURL);
        return;
    }

    GetWorld()->ServerTravel(TargetMapURL + TEXT("?listen"));
}