#include "Gimmicks/BaruElevatorActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "Player/BaruPlayerState.h"
#include "Core/BaruGameState.h"
#include "Core/BaruLobbyGameMode.h"
#include "Core/BaruGameMode.h"
#include "BaruLog.h"

ABaruElevatorActor::ABaruElevatorActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
    SetRootComponent(RootSceneComponent);

    PlatformMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMeshComponent"));
    PlatformMeshComponent->SetupAttachment(RootSceneComponent);
    PlatformMeshComponent->SetCollisionProfileName(TEXT("BlockAll"));

    // 플레이어 탑승 감지용 트리거 볼륨
    BoardingTriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BoardingTriggerBox"));
    BoardingTriggerBox->SetupAttachment(PlatformMeshComponent);
    BoardingTriggerBox->SetBoxExtent(FVector(150.0f, 150.0f, 100.0f));
    BoardingTriggerBox->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
    BoardingTriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    BoardingTriggerBox->SetGenerateOverlapEvents(true);
}

void ABaruElevatorActor::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority())
    {
        return;
    }

    CachedGameState = GetWorld()->GetGameState<ABaruGameState>();

    BoardingTriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ABaruElevatorActor::HandleTriggerBeginOverlap);
    BoardingTriggerBox->OnComponentEndOverlap.AddDynamic(this, &ABaruElevatorActor::HandleTriggerEndOverlap);
    
    // [레벨 시작 즉시는 엘리베이터 비활성화 상태
    bIsElevatorArmed = false;

    // 지정된 락아웃 시간 이후에만 엘리베이터 감지 활성화
    FTimerHandle LockoutTimerHandle;
    GetWorldTimerManager().SetTimer(
        LockoutTimerHandle,
        this,
        &ABaruElevatorActor::EnableElevatorActivation,
        ArrivalLockoutDuration,
        false
    );
}

void ABaruElevatorActor::EnableElevatorActivation()
{
    bIsElevatorArmed = true;
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Elevator System Armed & Ready for Boarding."));
}

void ABaruElevatorActor::HandleTriggerBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!HasAuthority() || !IsValid(OtherActor))
    {
        return;
    }

    APawn* PlayerPawn = Cast<APawn>(OtherActor);
    if (!PlayerPawn || !PlayerPawn->IsPlayerControlled())
    {
        return;
    }

    BoardedPlayers.Add(PlayerPawn);
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Player Entered Elevator: %s (Current: %d)"), *PlayerPawn->GetName(), BoardedPlayers.Num());

    // 전원 탑승 검사 & bIsElevatorArmed == true 일 경우에만 카운트다운 시작
    if (bIsElevatorArmed && !bIsCountingDown && CheckAllPlayersBoarded())
    {
        StartCountdown();
    }
}

void ABaruElevatorActor::HandleTriggerEndOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    if (!HasAuthority() || !IsValid(OtherActor))
    {
        return;
    }

    APawn* PlayerPawn = Cast<APawn>(OtherActor);
    if (!PlayerPawn)
    {
        return;
    }

    BoardedPlayers.Remove(PlayerPawn);
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Player Exited Elevator: %s (Current: %d)"), *PlayerPawn->GetName(), BoardedPlayers.Num());

    // 카운트다운 도중 1명이라도 이탈 시 즉시 취소
    if (bIsCountingDown && !CheckAllPlayersBoarded())
    {
        CancelCountdown();
    }
}

bool ABaruElevatorActor::CheckAllPlayersBoarded()
{
    int32 RequiredPlayers = 0;
    int32 ValidBoardedCount = 0;

    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        APlayerController* PC = Iterator->Get();
        if (IsValid(PC) && !PC->IsPendingKillPending())
        {
            if (const ABaruPlayerState* PS = PC->GetPlayerState<ABaruPlayerState>())
            {
                if (PS->IsAlive())
                {
                    RequiredPlayers++;

                    if (APawn* Pawn = PC->GetPawn())
                    {
                        if (BoardedPlayers.Contains(Pawn))
                        {
                            ValidBoardedCount++;
                        }
                    }
                }
            }
        }
    }

    return (RequiredPlayers > 0) && (ValidBoardedCount >= RequiredPlayers);
}

void ABaruElevatorActor::StartCountdown()
{
    bIsCountingDown = true;
    RemainingCountdown = CountdownDuration;

    if (!CachedGameState)
    {
        CachedGameState = GetWorld()->GetGameState<ABaruGameState>();
    }

    if (CachedGameState)
    {
        CachedGameState->Multicast_BroadcastNotification(
            FText::FromString(TEXT("ALL OPERATIVES ONBOARD - DEPARTING IN 5 SECONDS")), 3.0f);
    }

    GetWorldTimerManager().SetTimer(
        CountdownTimerHandle,
        this,
        &ABaruElevatorActor::UpdateCountdownTick,
        1.0f,
        true
    );

    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Elevator Countdown Started (%.0f seconds)"), CountdownDuration);
}

void ABaruElevatorActor::CancelCountdown()
{
    bIsCountingDown = false;
    RemainingCountdown = 0.0f;
    GetWorldTimerManager().ClearTimer(CountdownTimerHandle);

    if (CachedGameState)
    {
        CachedGameState->Multicast_BroadcastNotification(
            FText::FromString(TEXT("DEPARTURE CANCELLED - OPERATIVE LEFT THE ELEVATOR")), 2.0f);
    }

    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Elevator Countdown Cancelled."));
}

void ABaruElevatorActor::UpdateCountdownTick()
{
    RemainingCountdown -= 1.0f;

    if (RemainingCountdown > 0.0f)
    {
        if (CachedGameState)
        {
            CachedGameState->Multicast_BroadcastNotification(
                FText::Format(FText::FromString(TEXT("DEPARTING IN {0}...")), FText::AsNumber(FMath::RoundToInt(RemainingCountdown))), 1.0f);
        }
    }
    else
    {
        GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
        OnCountdownCompleted();
    }
}

void ABaruElevatorActor::OnCountdownCompleted()
{
    bIsCountingDown = false;

    // 목적지 맵 패키지 경로 추출 (하드코딩 방지)
    FString TargetMapURL = TEXT("");
    if (!DestinationLevel.IsNull())
    {
        TargetMapURL = DestinationLevel.ToSoftObjectPath().GetLongPackageName();
        if (TargetMapURL.IsEmpty())
        {
            TargetMapURL = DestinationLevel.GetAssetName();
        }
    }

    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Elevator Departing! Target Map: %s"), *TargetMapURL);

    AGameModeBase* AuthGameMode = GetWorld()->GetAuthGameMode();
    if (!IsValid(AuthGameMode))
    {
        return;
    }

    // 1) 로비 게임모드인 경우 (회사 로비 -> B1 던전 이동)
    if (ABaruLobbyGameMode* LobbyGM = Cast<ABaruLobbyGameMode>(AuthGameMode))
    {
        LobbyGM->StartGameRaid(TargetMapURL);
        return;
    }

    // 2) 인게임 탐사 게임모드인 경우 (B1 던전 탈출 -> 회사 로비 복귀)
    if (ABaruGameMode* IngameGM = Cast<ABaruGameMode>(AuthGameMode))
    {
        IngameGM->RequestLevelTransition(TargetMapURL);
        return;
    }
}