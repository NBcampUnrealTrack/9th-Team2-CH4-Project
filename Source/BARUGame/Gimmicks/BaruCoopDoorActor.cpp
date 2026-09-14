#include "Gimmicks/BaruCoopDoorActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Gimmicks/BaruCoopButtonActor.h"
#include "GameFramework/Pawn.h"
#include "BaruLog.h"

ABaruCoopDoorActor::ABaruCoopDoorActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false; // 기본 정지 상태에서는 틱 비활성화
    bReplicates = true;

    RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    SetRootComponent(RootScene);

    FrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh"));
    FrameMesh->SetupAttachment(RootScene);

    ShutterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShutterMesh"));
    ShutterMesh->SetupAttachment(FrameMesh);
    ShutterMesh->SetCollisionProfileName(TEXT("BlockAll"));
    
    DoorMovementAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("DoorMovementAudioComp"));
    DoorMovementAudioComp->SetupAttachment(ShutterMesh);
    DoorMovementAudioComp->bAutoActivate = false;
}

void ABaruCoopDoorActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ABaruCoopDoorActor, DoorState);
    DOREPLIFETIME(ABaruCoopDoorActor, ReplicatedShutterLoc);
}

void ABaruCoopDoorActor::BeginPlay()
{
    Super::BeginPlay();

    InitialShutterLoc = ShutterMesh->GetRelativeLocation();
    ReplicatedShutterLoc = InitialShutterLoc;
}

void ABaruCoopDoorActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    const FVector CurrentLocation = ShutterMesh->GetRelativeLocation();

    // 1. 상승 상태 (2명 누름)
    if (DoorState == EBaruCoopDoorState::Opening)
    {
        const FVector MaxLiftLoc = InitialShutterLoc + FVector(0.0f, 0.0f, LiftHeight);
        const FVector NewLocation = FMath::VInterpConstantTo(CurrentLocation, MaxLiftLoc, DeltaTime, LiftSpeed);
        ShutterMesh->SetRelativeLocation(NewLocation);

        if (HasAuthority())
        {
            ReplicatedShutterLoc = NewLocation;

            // 끝까지 도달하면 정지
            if (NewLocation.Equals(MaxLiftLoc, 0.5f))
            {
                ShutterMesh->SetRelativeLocation(MaxLiftLoc);
                ReplicatedShutterLoc = MaxLiftLoc;
                SetDoorMovementState(EBaruCoopDoorState::Stopped);
            }
        }
    }
    // 2. 하강 상태 (0명 누름)
    else if (DoorState == EBaruCoopDoorState::Closing)
    {
        const FVector NewLocation = FMath::VInterpConstantTo(CurrentLocation, InitialShutterLoc, DeltaTime, LowerSpeed);
        ShutterMesh->SetRelativeLocation(NewLocation);

        if (HasAuthority())
        {
            ReplicatedShutterLoc = NewLocation;

            // 바닥에 완전히 닿으면 정지
            if (NewLocation.Equals(InitialShutterLoc, 0.5f))
            {
                ShutterMesh->SetRelativeLocation(InitialShutterLoc);
                ReplicatedShutterLoc = InitialShutterLoc;
                SetDoorMovementState(EBaruCoopDoorState::Stopped);
            }
        }
    }
}

bool ABaruCoopDoorActor::CanAcceptButtonPress(const ABaruCoopButtonActor* InButton, const APawn* Interactor) const
{
    // 동일 플레이어가 2개 버튼 동시 점유 방지
    if (ActiveInteractors.Contains(Interactor))
    {
        return false;
    }

    return true;
}

void ABaruCoopDoorActor::NotifyButtonPressed(ABaruCoopButtonActor* InButton, APawn* Interactor)
{
    if (!HasAuthority() || !IsValid(InButton) || !IsValid(Interactor))
    {
        return;
    }

    if (ActiveInteractors.Contains(Interactor) || ActiveButtons.Contains(InButton))
    {
        return;
    }

    ActiveButtons.Add(InButton);
    ActiveInteractors.Add(Interactor);

    EvaluateDoorMovement();
}

void ABaruCoopDoorActor::NotifyButtonReleased(ABaruCoopButtonActor* InButton, APawn* Interactor)
{
    if (!HasAuthority() || !IsValid(InButton))
    {
        return;
    }

    ActiveButtons.Remove(InButton);
    ActiveInteractors.Remove(Interactor);

    EvaluateDoorMovement();
}

void ABaruCoopDoorActor::EvaluateDoorMovement()
{
    const int32 ActiveCount = ActiveButtons.Num();
    const FVector CurrentLoc = ShutterMesh->GetRelativeLocation();

    // 2명이 누름 -> 상승
    if (ActiveCount >= 2)
    {
        const FVector MaxLiftLoc = InitialShutterLoc + FVector(0.0f, 0.0f, LiftHeight);
        if (!CurrentLoc.Equals(MaxLiftLoc, 0.5f))
        {
            SetDoorMovementState(EBaruCoopDoorState::Opening);
        }
        else
        {
            SetDoorMovementState(EBaruCoopDoorState::Stopped);
        }
    }
    // 1명이 누름 -> 그 자리에 정지
    else if (ActiveCount == 1)
    {
        SetDoorMovementState(EBaruCoopDoorState::Stopped);
    }
    // 0명이 누름 -> 바닥으로 하강
    else
    {
        if (!CurrentLoc.Equals(InitialShutterLoc, 0.5f))
        {
            SetDoorMovementState(EBaruCoopDoorState::Closing);
        }
        else
        {
            SetDoorMovementState(EBaruCoopDoorState::Stopped);
        }
    }
}

void ABaruCoopDoorActor::SetDoorMovementState(EBaruCoopDoorState NewState)
{
    if (!HasAuthority() || DoorState == NewState)
    {
        return;
    }

    DoorState = NewState;
    OnRep_DoorState(); // 서버/호스트 로컬 처리
}

void ABaruCoopDoorActor::OnRep_DoorState()
{
    if (DoorState == EBaruCoopDoorState::Stopped)
    {
        SetActorTickEnabled(false);

        // [09.13 수정] 이동 중지 시 루프 사운드 종료 및 3D 쿵 닫힘/정지음 출력
        if (DoorMovementAudioComp && DoorMovementAudioComp->IsPlaying())
        {
            DoorMovementAudioComp->Stop();
        }

        if (DoorStopSound && ShutterMesh)
        {
            UGameplayStatics::PlaySoundAtLocation(
                this,
                DoorStopSound,
                ShutterMesh->GetComponentLocation(),
                1.0f,
                1.0f,
                0.0f,
                DoorAudioAttenuation
            );
        }
    }
    else
    {
        SetActorTickEnabled(true);

        // 문이 열리거나 닫히기 시작할 때 셔터 위치에서 3D 모터/체인 루핑 사운드 시작
        if (DoorMovementAudioComp && DoorMovingLoopSound)
        {
            if (!DoorMovementAudioComp->IsPlaying())
            {
                DoorMovementAudioComp->SetSound(DoorMovingLoopSound);
                DoorMovementAudioComp->AttenuationSettings = DoorAudioAttenuation;
                DoorMovementAudioComp->Play();
            }
        }
    }

    BP_OnDoorMovementStateChanged(DoorState);
}

void ABaruCoopDoorActor::OnRep_ShutterLoc()
{
    // 서버와 위치 오차 동기화 보정
    ShutterMesh->SetRelativeLocation(ReplicatedShutterLoc);
}