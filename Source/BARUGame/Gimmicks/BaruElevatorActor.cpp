#include "Gimmicks/BaruElevatorActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Player/BaruPlayerState.h"
#include "Core/BaruGameState.h"
#include "Core/BaruLobbyGameMode.h"
#include "Core/BaruGameMode.h"
#include "GameplayTags/BaruGameplayTags.h"
#include "BaruLog.h"

ABaruElevatorActor::ABaruElevatorActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bAlwaysRelevant = true;

    RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
    SetRootComponent(RootSceneComponent);

    PlatformMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMeshComponent"));
    PlatformMeshComponent->SetupAttachment(RootSceneComponent);
    PlatformMeshComponent->SetCollisionProfileName(TEXT("BlockAll"));

    ConsoleSwitchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ConsoleSwitchMesh"));
    ConsoleSwitchMesh->SetupAttachment(PlatformMeshComponent);
    ConsoleSwitchMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    ConsoleSwitchMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block); // Interaction 채널

    BoardingTriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BoardingTriggerBox"));
    BoardingTriggerBox->SetupAttachment(PlatformMeshComponent);
    BoardingTriggerBox->SetBoxExtent(FVector(150.0f, 150.0f, 100.0f));
    BoardingTriggerBox->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
    BoardingTriggerBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    BoardingTriggerBox->SetGenerateOverlapEvents(true);
}

void ABaruElevatorActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ABaruElevatorActor, bIsCountingDown);
    DOREPLIFETIME(ABaruElevatorActor, bIsDeparted);
    DOREPLIFETIME(ABaruElevatorActor, RemainingCountdown);
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

    bIsElevatorArmed = false;
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
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Elevator System Armed & Ready."));
}

bool ABaruElevatorActor::CanInteract_Implementation(APawn* Interactor) const
{
    if (!bIsElevatorArmed || bIsDeparted || bIsCountingDown)
    {
        return false;
    }

    if (TriggerType == EBaruElevatorTriggerType::ManualInteract)
    {
        return CheckAllPlayersBoarded();
    }

    return false;
}

FText ABaruElevatorActor::GetInteractPromptText_Implementation(APawn* Interactor) const
{
    if (CheckAllPlayersBoarded())
    {
        return FText::FromString(TEXT("F: 엘리베이터 가동"));
    }
    return FText::FromString(TEXT("팀원 전원 탑승 대기 중..."));
}

FGameplayTag ABaruElevatorActor::GetInteractionTag_Implementation() const
{
    return FBaruGameplayTags::Get().Interaction_Type_Elevator;
}

float ABaruElevatorActor::GetInteractionDuration_Implementation() const
{
    return 0.0f;
}

void ABaruElevatorActor::ExecuteInteraction_Implementation(APawn* Interactor)
{
    if (!HasAuthority() || bIsDeparted || bIsCountingDown)
    {
        return;
    }

    if (CheckAllPlayersBoarded())
    {
        StartCountdown();
    }
}

void ABaruElevatorActor::HandleTriggerBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!HasAuthority() || bIsDeparted || !IsValid(OtherActor)) return;

    APawn* PlayerPawn = Cast<APawn>(OtherActor);
    if (!PlayerPawn || !PlayerPawn->IsPlayerControlled()) return;

    BoardedPlayers.Add(PlayerPawn);
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Player Entered Elevator: %s (Count: %d)"), *PlayerPawn->GetName(), BoardedPlayers.Num());

    if (TriggerType == EBaruElevatorTriggerType::AutoOnAllBoarded && bIsElevatorArmed && !bIsCountingDown)
    {
        if (CheckAllPlayersBoarded())
        {
            StartCountdown();
        }
    }
}

void ABaruElevatorActor::HandleTriggerEndOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    if (!HasAuthority() || bIsDeparted || !IsValid(OtherActor)) return;

    APawn* PlayerPawn = Cast<APawn>(OtherActor);
    if (!PlayerPawn) return;

    BoardedPlayers.Remove(PlayerPawn);
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Player Exited Elevator: %s (Count: %d)"), *PlayerPawn->GetName(), BoardedPlayers.Num());

    if (bIsCountingDown && !CheckAllPlayersBoarded())
    {
        CancelCountdown();
    }
}

bool ABaruElevatorActor::CheckAllPlayersBoarded() const
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
    OnRep_IsCountingDown();

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

    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Elevator Countdown Started (%.0f sec)."), CountdownDuration);
}

void ABaruElevatorActor::CancelCountdown()
{
    bIsCountingDown = false;
    RemainingCountdown = 0.0f;
    GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
    OnRep_IsCountingDown();

    if (CachedGameState)
    {
        CachedGameState->Multicast_BroadcastNotification(
            FText::FromString(TEXT("DEPARTURE CANCELLED - OPERATIVE LEFT")), 2.0f);
    }

    BP_OnCountdownCancelled();
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
    bIsDeparted = true;
    OnRep_IsDeparted();

    FString TargetMapURL = TEXT("");
    if (!DestinationLevel.IsNull())
    {
        TargetMapURL = DestinationLevel.ToSoftObjectPath().GetLongPackageName();
        if (TargetMapURL.IsEmpty())
        {
            TargetMapURL = DestinationLevel.GetAssetName();
        }
    }

    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Elevator Departing to: %s"), *TargetMapURL);

    AGameModeBase* AuthGameMode = GetWorld()->GetAuthGameMode();
    if (!IsValid(AuthGameMode)) return;

    if (ABaruLobbyGameMode* LobbyGM = Cast<ABaruLobbyGameMode>(AuthGameMode))
    {
        LobbyGM->StartGameRaid(TargetMapURL);
        return;
    }

    if (ABaruGameMode* IngameGM = Cast<ABaruGameMode>(AuthGameMode))
    {
        IngameGM->RequestLevelTransition(TargetMapURL);
        return;
    }
}

void ABaruElevatorActor::OnRep_IsCountingDown()
{
    if (bIsCountingDown)
    {
        BP_OnCountdownStarted(CountdownDuration);
    }
    else if (!bIsDeparted)
    {
        BP_OnCountdownCancelled();
    }
}

void ABaruElevatorActor::OnRep_IsDeparted()
{
    if (bIsDeparted)
    {
        BP_OnDeparted();
    }
}