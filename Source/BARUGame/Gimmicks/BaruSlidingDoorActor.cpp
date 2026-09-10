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
    PrimaryActorTick.bStartWithTickEnabled = false;
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

    // 에디터에 배치된 초기 위치 캐싱
    InitialLeftDoorLoc = LeftDoorMesh->GetRelativeLocation();
    InitialRightDoorLoc = RightDoorMesh->GetRelativeLocation();
}

void ABaruSlidingDoorActor::StartDoorMotion()
{
    // 움직임 시작 시 틱 활성화
    SetActorTickEnabled(true);
}

void ABaruSlidingDoorActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    const FVector TargetLeftLoc = bIsOpen ? (InitialLeftDoorLoc + FVector(SlideDistance, 0.0f, 0.0f)) : InitialLeftDoorLoc;
    const FVector TargetRightLoc = bIsOpen ? (InitialRightDoorLoc + FVector(-SlideDistance, 0.0f, 0.0f)) : InitialRightDoorLoc;

    const FVector CurrentLeftLoc = LeftDoorMesh->GetRelativeLocation();
    const FVector CurrentRightLoc = RightDoorMesh->GetRelativeLocation();

    const FVector NewLeftLoc = FMath::VInterpTo(CurrentLeftLoc, TargetLeftLoc, DeltaTime, SlideSpeed);
    const FVector NewRightLoc = FMath::VInterpTo(CurrentRightLoc, TargetRightLoc, DeltaTime, SlideSpeed);

    LeftDoorMesh->SetRelativeLocation(NewLeftLoc);
    RightDoorMesh->SetRelativeLocation(NewRightLoc);
    
    //  문이 모두 목표 위치에 도달하면 위치를 확정하고 틱 자동 비활성화
    if (NewLeftLoc.Equals(TargetLeftLoc, 0.1f) && NewRightLoc.Equals(TargetRightLoc, 0.1f))
    {
        LeftDoorMesh->SetRelativeLocation(TargetLeftLoc);
        RightDoorMesh->SetRelativeLocation(TargetRightLoc);
        SetActorTickEnabled(false);
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
    StartDoorMotion();
    BP_OnDoorStateChanged(bIsOpen);
}