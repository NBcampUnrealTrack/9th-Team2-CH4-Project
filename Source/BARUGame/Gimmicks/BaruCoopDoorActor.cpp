// BaruCoopDoorActor.cpp
#include "Gimmicks/BaruCoopDoorActor.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Gimmicks/BaruCoopButtonActor.h"
#include "GameFramework/Pawn.h"
#include "BaruLog.h"

ABaruCoopDoorActor::ABaruCoopDoorActor()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    SetRootComponent(RootScene);

    FrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrameMesh"));
    FrameMesh->SetupAttachment(RootScene);

    ShutterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShutterMesh"));
    ShutterMesh->SetupAttachment(FrameMesh);
    ShutterMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ABaruCoopDoorActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ABaruCoopDoorActor, bIsOpen);
}

void ABaruCoopDoorActor::BeginPlay()
{
    Super::BeginPlay();

    // [중요] 에디터에 배치된 셔터의 초기 상대 위치 보존
    InitialShutterLoc = ShutterMesh->GetRelativeLocation();
}

void ABaruCoopDoorActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 초기 Z좌표 기준 상향 보간
    const FVector TargetLocation = bIsOpen ? (InitialShutterLoc + FVector(0.0f, 0.0f, LiftHeight)) : InitialShutterLoc;
    const FVector CurrentLocation = ShutterMesh->GetRelativeLocation();

    if (!CurrentLocation.Equals(TargetLocation, 0.1f))
    {
        ShutterMesh->SetRelativeLocation(FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, LiftSpeed));
    }
}

bool ABaruCoopDoorActor::CanAcceptButtonPress(const ABaruCoopButtonActor* InButton, const APawn* Interactor) const
{
    if (bIsOpen) return false;

    if (FirstInteractingPlayer.IsValid() && FirstInteractingPlayer.Get() == Interactor)
    {
        return false;
    }

    return true;
}

void ABaruCoopDoorActor::NotifyButtonPressed(ABaruCoopButtonActor* InButton, APawn* Interactor)
{
    if (!HasAuthority() || bIsOpen || !IsValid(InButton) || !IsValid(Interactor))
    {
        return;
    }

    // 1단계: 첫 번째 버튼 활성화
    if (!FirstPressedButton.IsValid())
    {
        FirstPressedButton = InButton;
        FirstInteractingPlayer = Interactor;
        InButton->SetButtonActive(true);

        GetWorldTimerManager().SetTimer(
            ButtonTimeoutTimerHandle,
            this,
            &ABaruCoopDoorActor::HandleButtonTimeout,
            SyncToleranceSeconds,
            false
        );

        Multicast_OnFirstButtonActivated(SyncToleranceSeconds);
        BARU_NET_LOG(this, LogBaruSession, Log, TEXT("CoopShutter: Button 1 activated by %s"), *Interactor->GetName());
        return;
    }

    // 2단계: 1인 2버튼 치팅 및 동일 버튼 중복 입력 방어
    if (FirstPressedButton.Get() == InButton || FirstInteractingPlayer.Get() == Interactor)
    {
        return;
    }

    // 동시 인증 성공: 차고문 개방
    GetWorldTimerManager().ClearTimer(ButtonTimeoutTimerHandle);
    InButton->SetButtonActive(true);

    bIsOpen = true;
    OnRep_IsOpen();

    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("CoopShutter: Sync verified! Shutter lifting."));
}

void ABaruCoopDoorActor::HandleButtonTimeout()
{
    if (!HasAuthority() || bIsOpen) return;

    if (FirstPressedButton.IsValid())
    {
        FirstPressedButton->SetButtonActive(false);
    }

    FirstPressedButton.Reset();
    FirstInteractingPlayer.Reset();

    Multicast_OnSyncFailed();
    BARU_NET_LOG(this, LogBaruSession, Log, TEXT("CoopShutter: Button sync timeout!"));
}

void ABaruCoopDoorActor::Multicast_OnFirstButtonActivated_Implementation(float TimeRemaining)
{
    BP_OnFirstButtonActivated(TimeRemaining);
}

void ABaruCoopDoorActor::Multicast_OnSyncFailed_Implementation()
{
    BP_OnSyncFailed();
}

void ABaruCoopDoorActor::OnRep_IsOpen()
{
    BP_OnDoorStateChanged(bIsOpen);
}