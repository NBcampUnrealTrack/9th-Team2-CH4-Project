#include "Gimmicks/BaruElevatorActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Player/BaruPlayerState.h"
#include "Character/BaruCharacter.h"
#include "Interfaces/CombatInterface.h"
#include "Core/BaruGameState.h"
#include "Core/BaruLobbyGameMode.h"
#include "Core/BaruGameMode.h"
#include "GameplayTags/BaruGameplayTags.h"
#include "Kismet/GameplayStatics.h"
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
    PlatformMeshComponent->CanCharacterStepUpOn = ECB_Yes;

    ConsoleSwitchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ConsoleSwitchMesh"));
    ConsoleSwitchMesh->SetupAttachment(PlatformMeshComponent);
    ConsoleSwitchMesh->SetRelativeLocation(FVector(120.0f, 0.0f, 95.0f)); 
    ConsoleSwitchMesh->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
    ConsoleSwitchMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    ConsoleSwitchMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);

    FloorGuardCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("FloorGuardCollision"));
    FloorGuardCollision->SetupAttachment(PlatformMeshComponent);
    FloorGuardCollision->SetBoxExtent(FVector(180.0f, 180.0f, 25.0f));
    FloorGuardCollision->SetRelativeLocation(FVector(0.0f, 0.0f, -25.0f));
    FloorGuardCollision->SetCollisionProfileName(TEXT("BlockAll"));
    FloorGuardCollision->CanCharacterStepUpOn = ECB_Yes;
    FloorGuardCollision->SetGenerateOverlapEvents(false);
    
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

bool ABaruElevatorActor::IsPlayerBoarded(const APawn* Interactor) const
{
    if (!IsValid(Interactor)) return false;
    
    if (HasAuthority())
    {
        return BoardedPlayers.Contains(Interactor);
    }
    
    if (IsValid(BoardingTriggerBox))
    {
        return BoardingTriggerBox->IsOverlappingActor(Interactor);
    }

    return false;
}

bool ABaruElevatorActor::CanInteract_Implementation(APawn* Interactor) const
{
    if (!bIsElevatorArmed || bIsDeparted)
    {
        return false;
    }

    // 발판 위에 서 있는 사람만 조작 허용
    if (!IsPlayerBoarded(Interactor))
    {
        return false;
    }

    // 로비 레벨: 전원이 탑승해야만 작동 가능
    if (UWorld* World = GetWorld())
    {
        if (Cast<ABaruLobbyGameMode>(World->GetAuthGameMode()))
        {
            if (bIsCountingDown) return false;
            return CheckAllPlayersBoarded();
        }
    }

    // 인게임 레벨: 언제든 비상 출발/취소 토글 가능
    return true;
}

FText ABaruElevatorActor::GetInteractPromptText_Implementation(APawn* Interactor) const
{
    UWorld* World = GetWorld();
    AGameModeBase* AuthGM = World ? World->GetAuthGameMode() : nullptr;

    if (Cast<ABaruLobbyGameMode>(AuthGM))
    {
        if (CheckAllPlayersBoarded())
        {
            return FText::FromString(TEXT("F: 탐사 시작"));
        }
        return FText::FromString(TEXT("팀원 전원 탑승 대기 중..."));
    }
    
    // 인게임 프롬프트
    if (bIsCountingDown)
    {
        return FText::FromString(TEXT("F: 엘리베이터 출발 취소"));
    }

    if (CheckAllPlayersBoarded())
    {
        return FText::FromString(TEXT("F: 엘리베이터 출발 (10초)"));
    }

    return FText::FromString(TEXT("F: 비상 출발 [미탑승 대원 낙오] (10초)"));
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
    if (!HasAuthority() || bIsDeparted)
    {
        return;
    }

    if (!BoardedPlayers.Contains(Interactor))
    {
        return;
    }

    // 버튼 연타 스팸 방지
    const float CurrentTime = GetWorld()->GetTimeSeconds();
    if (CurrentTime - LastInteractionTime < InteractionDebounceDelay)
    {
        return;
    }
    LastInteractionTime = CurrentTime;

    Multicast_PlayButtonSound();
    
    AGameModeBase* AuthGM = GetWorld()->GetAuthGameMode();

    // 로비: 전원 탑승 시 시작
    if (Cast<ABaruLobbyGameMode>(AuthGM))
    {
        if (CheckAllPlayersBoarded() && !bIsCountingDown)
        {
            StartCountdown();
        }
        return;
    }

    // 인게임: 시작 <-> 취소 토글
    if (bIsCountingDown)
    {
        CancelCountdown();
    }
    else
    {
        StartCountdown();
    }
}

void ABaruElevatorActor::Multicast_PlayButtonSound_Implementation()
{
    if (ButtonInteractSound && ConsoleSwitchMesh)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            ButtonInteractSound,
            ConsoleSwitchMesh->GetComponentLocation(),
            1.0f,
            1.0f,
            0.0f,
            ButtonAudioAttenuation
        );
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

    if (bIsCountingDown)
    {
        AGameModeBase* AuthGM = GetWorld()->GetAuthGameMode();
        
        // 로비에서는 1명이라도 이탈 시 취소
        if (Cast<ABaruLobbyGameMode>(AuthGM))
        {
            if (!CheckAllPlayersBoarded())
            {
                CancelCountdown();
            }
        }
        else
        {
            // 인게임에서는 엘리베이터에 탄 인원이 0명이 되었을 때만 자동 취소 (누군가 타고 있다면 계속 유지)
            if (BoardedPlayers.Num() == 0)
            {
                CancelCountdown();
            }
        }
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
    if (bIsCountingDown || bIsDeparted) return;

    bIsCountingDown = true;

    AGameModeBase* AuthGM = GetWorld()->GetAuthGameMode();
    const bool bIsLobby = (Cast<ABaruLobbyGameMode>(AuthGM) != nullptr);

    RemainingCountdown = bIsLobby ? CountdownDuration : IngameCountdownDuration;
    OnRep_IsCountingDown();

    if (!CachedGameState)
    {
        CachedGameState = GetWorld()->GetGameState<ABaruGameState>();
    }

    if (CachedGameState)
    {
        const FText StartMsg = FText::Format(
            FText::FromString(TEXT("{0}초 후 엘리베이터가 출발합니다!")),
            FText::AsNumber(FMath::RoundToInt(RemainingCountdown))
        );
        CachedGameState->Multicast_BroadcastNotification(StartMsg, 2.0f);
    }

    GetWorldTimerManager().SetTimer(
        CountdownTimerHandle,
        this,
        &ABaruElevatorActor::UpdateCountdownTick,
        1.0f,
        true
    );

    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("Elevator Countdown Started (%.0f sec)."), RemainingCountdown);
}

void ABaruElevatorActor::CancelCountdown()
{
    if (!bIsCountingDown || bIsDeparted) return;

    bIsCountingDown = false;
    RemainingCountdown = 0.0f;
    GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
    OnRep_IsCountingDown();

    if (CachedGameState)
    {
        CachedGameState->Multicast_BroadcastNotification(
            FText::FromString(TEXT("엘리베이터 출발이 취소되었습니다.")), 2.0f);
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
            const FText TickMsg = FText::Format(
                FText::FromString(TEXT("{0}초 후 엘리베이터가 출발합니다!")),
                FText::AsNumber(FMath::RoundToInt(RemainingCountdown))
            );
            CachedGameState->Multicast_BroadcastNotification(TickMsg, 1.0f);
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
    // 중복 실행 차단
    if (bIsDeparted) return;

    bIsCountingDown = false;
    bIsDeparted = true;

    // 출발 확정 즉시 모든 트리거/콘솔 충돌을 꺼서 8초 대기 중 추가 상호작용 및 오버랩 원천 차단
    if (BoardingTriggerBox) BoardingTriggerBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (ConsoleSwitchMesh) ConsoleSwitchMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

    // 로비 게임모드인 경우
    if (ABaruLobbyGameMode* LobbyGM = Cast<ABaruLobbyGameMode>(AuthGameMode))
    {
        LobbyGM->StartGameRaid(TargetMapURL);
        return;
    }

    // 인게임 게임모드인 경우
    if (ABaruGameMode* IngameGM = Cast<ABaruGameMode>(AuthGameMode))
    {
        bool bHasLeftBehind = false;

        // 외부 대원 낙오 처리
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            APlayerController* PC = It->Get();
            if (!IsValid(PC) || PC->IsPendingKillPending()) continue;

            APawn* PlayerPawn = PC->GetPawn();
            ABaruPlayerState* PS = PC->GetPlayerState<ABaruPlayerState>();

            // 엘리베이터 탑승 목록에 없는 인원 선별
            if (!BoardedPlayers.Contains(PlayerPawn))
            {
                bHasLeftBehind = true;

                if (IsValid(PlayerPawn) && PlayerPawn->Implements<UCombatInterface>())
                {
                    if (!ICombatInterface::Execute_IsDead(PlayerPawn))
                    {
                        ICombatInterface::Execute_Die(PlayerPawn, this);
                        BARU_NET_LOG(this, LogBaruSession, Warning, 
                            TEXT("Operative left behind: %s (Killed upon elevator departure)"), *PlayerPawn->GetName());
                    }
                }
                else if (PS && !PS->IsDead())
                {
                    PS->SetDBNOState(false);
                    PS->SetDeadState(true);
                }
            }
        }

        if (bHasLeftBehind && CachedGameState)
        {
            CachedGameState->Multicast_BroadcastNotification(
                FText::FromString(TEXT("엘리베이터가 출발했습니다. 미탑승 대원은 낙오되었습니다.")), 4.0f);
        }
        
        IngameGM->RequestLevelTransition(TargetMapURL);
        return;
    }
}

void ABaruElevatorActor::OnRep_IsCountingDown()
{
    if (bIsCountingDown)
    {
        const float SafeDuration = (RemainingCountdown > 0.0f) ? RemainingCountdown : CountdownDuration;
        BP_OnCountdownStarted(SafeDuration);
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