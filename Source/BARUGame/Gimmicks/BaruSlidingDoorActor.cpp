// BaruSlidingDoorActor.cpp
#include "Gimmicks/BaruSlidingDoorActor.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "GameplayTags/BaruGameplayTags.h"
#include "BaruLog.h"

ABaruSlidingDoorActor::ABaruSlidingDoorActor()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;

    RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
    SetRootComponent(RootScene);

    DoorFrameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorFrameMesh"));
    DoorFrameMesh->SetupAttachment(RootScene);

    LeftDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftDoorMesh"));
    LeftDoorMesh->SetupAttachment(DoorFrameMesh);
    LeftDoorMesh->SetCollisionProfileName(TEXT("BlockAll"));
    LeftDoorMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);

    RightDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightDoorMesh"));
    RightDoorMesh->SetupAttachment(DoorFrameMesh);
    RightDoorMesh->SetCollisionProfileName(TEXT("BlockAll"));
    RightDoorMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);
}

void ABaruSlidingDoorActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ABaruSlidingDoorActor, bIsOpen);
}

void ABaruSlidingDoorActor::BeginPlay()
{
    Super::BeginPlay();

    // [중요] 에디터에서 배치한 원래 닫힌 위치를 기준 좌표로 캐싱
    InitialLeftDoorLoc = LeftDoorMesh->GetRelativeLocation();
    InitialRightDoorLoc = RightDoorMesh->GetRelativeLocation();
}

void ABaruSlidingDoorActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 초기 위치 기준 오프셋 계산 (ZeroVector 버그 수정)
    const FVector TargetLeftLoc = bIsOpen ? (InitialLeftDoorLoc + FVector(0.0f, -SlideDistance, 0.0f)) : InitialLeftDoorLoc;
    const FVector TargetRightLoc = bIsOpen ? (InitialRightDoorLoc + FVector(0.0f, SlideDistance, 0.0f)) : InitialRightDoorLoc;

    const FVector CurrentLeftLoc = LeftDoorMesh->GetRelativeLocation();
    const FVector CurrentRightLoc = RightDoorMesh->GetRelativeLocation();

    if (!CurrentLeftLoc.Equals(TargetLeftLoc, 0.1f) || !CurrentRightLoc.Equals(TargetRightLoc, 0.1f))
    {
        LeftDoorMesh->SetRelativeLocation(FMath::VInterpTo(CurrentLeftLoc, TargetLeftLoc, DeltaTime, SlideSpeed));
        RightDoorMesh->SetRelativeLocation(FMath::VInterpTo(CurrentRightLoc, TargetRightLoc, DeltaTime, SlideSpeed));
    }
}

bool ABaruSlidingDoorActor::CanInteract_Implementation(APawn* Interactor) const
{
    if (bIsLocked) return false;
    if (bIsOpen && !bCanToggleClose) return false;
    return true;
}

FText ABaruSlidingDoorActor::GetInteractPromptText_Implementation(APawn* Interactor) const
{
    if (bIsLocked) return FText::FromString(TEXT("잠김"));
    return bIsOpen ? FText::FromString(TEXT("F: 문 닫기")) : FText::FromString(TEXT("F: 문 열기"));
}

FGameplayTag ABaruSlidingDoorActor::GetInteractionTag_Implementation() const
{
    return FBaruGameplayTags::Get().Interaction_Type_Door;
}

float ABaruSlidingDoorActor::GetInteractionDuration_Implementation() const
{
    return 0.0f;
}

void ABaruSlidingDoorActor::ExecuteInteraction_Implementation(APawn* Interactor)
{
    if (!HasAuthority()) return;

    bIsOpen = !bIsOpen;
    OnRep_IsOpen();

    BARU_NET_LOG(this, LogBaruItem, Log, TEXT("SlidingDoor State Changed -> %s by %s"), 
        bIsOpen ? TEXT("OPEN") : TEXT("CLOSED"), *GetNameSafe(Interactor));
}

void ABaruSlidingDoorActor::OnRep_IsOpen()
{
    BP_OnDoorStateChanged(bIsOpen);
}