#include "Gimmicks/BaruCoopButtonActor.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Gimmicks/BaruCoopDoorActor.h"
#include "GameplayTags/BaruGameplayTags.h"
#include "GameFramework/Pawn.h"
#include "BaruLog.h"

ABaruCoopButtonActor::ABaruCoopButtonActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    SetRootComponent(RootScene);

    ButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ButtonMesh"));
    ButtonMesh->SetupAttachment(RootScene);
    ButtonMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    ButtonMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block); // Interaction 채널
}

void ABaruCoopButtonActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ABaruCoopButtonActor, bIsPressed);
}

bool ABaruCoopButtonActor::CanInteract_Implementation(APawn* Interactor) const
{
    // 이미 눌려있거나 연결된 문이 없거나 문이 이미 열려있으면 상호작용 차단
    if (bIsPressed || !IsValid(TargetDoor))
    {
        return false;
    }

    return TargetDoor->CanAcceptButtonPress(this, Interactor);
}

FText ABaruCoopButtonActor::GetInteractPromptText_Implementation(APawn* Interactor) const
{
    return FText::FromString(TEXT("F: 동시 인증 버튼 누르기"));
}

FGameplayTag ABaruCoopButtonActor::GetInteractionTag_Implementation() const
{
    return FBaruGameplayTags::Get().Interaction_Type_CoOp;
}

float ABaruCoopButtonActor::GetInteractionDuration_Implementation() const
{
    return 0.0f;
}

void ABaruCoopButtonActor::ExecuteInteraction_Implementation(APawn* Interactor)
{
    if (!HasAuthority() || !IsValid(TargetDoor) || !IsValid(Interactor))
    {
        return;
    }

    HoldingPlayer = Interactor;
    SetButtonActive(true);

    TargetDoor->NotifyButtonPressed(this, Interactor);

    GetWorldTimerManager().SetTimer(
        HoldCheckTimerHandle,
        this,
        &ABaruCoopButtonActor::CheckHoldingPlayerValidity,
        0.1f,
        true
    );
}

void ABaruCoopButtonActor::EndInteraction_Implementation(APawn* Interactor)
{
    if (!HasAuthority() || !bIsPressed)
    {
        return;
    }

    GetWorldTimerManager().ClearTimer(HoldCheckTimerHandle);

    APawn* ReleasingPlayer = HoldingPlayer.Get();
    HoldingPlayer.Reset();

    SetButtonActive(false);

    if (IsValid(TargetDoor))
    {
        TargetDoor->NotifyButtonReleased(this, ReleasingPlayer);
    }
}

void ABaruCoopButtonActor::CheckHoldingPlayerValidity()
{
    if (!HasAuthority() || !bIsPressed) return;

    if (!HoldingPlayer.IsValid() ||
        FVector::DistSquared(GetActorLocation(), HoldingPlayer->GetActorLocation()) > FMath::Square(MaxHoldDistance))
    {
        EndInteraction_Implementation(HoldingPlayer.Get());
    }
}

void ABaruCoopButtonActor::SetButtonActive(bool bActive)
{
    if (!HasAuthority()) return;

    bIsPressed = bActive;
    OnRep_IsPressed();
}

void ABaruCoopButtonActor::OnRep_IsPressed()
{
    BP_OnButtonPressedStateChanged(bIsPressed);
}