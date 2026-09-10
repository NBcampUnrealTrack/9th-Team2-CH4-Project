#include "Gameplay/Items/Spawning/BaruCorpseLootActor.h"

#include "Components/SphereComponent.h"
#include "Gameplay/Items/Spawning/BaruMonsterItemSpawnerComponent.h"
#include "GameplayTags/BaruGameplayTags.h"
#include "Net/UnrealNetwork.h"

ABaruCorpseLootActor::ABaruCorpseLootActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bNetUseOwnerRelevancy = true;
    SetReplicateMovement(true);

    InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
    SetRootComponent(InteractionSphere);
    InteractionSphere->InitSphereRadius(80.0f);
    InteractionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    // BaruBaseItem / BaruCharacter와 같은 상호작용 전용 채널.
    InteractionSphere->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);
    InteractionSphere->SetGenerateOverlapEvents(false);
    InteractionSphere->SetCanEverAffectNavigation(false);
    InteractionSphere->SetAbsolute(false, false, true);
}

void ABaruCorpseLootActor::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ABaruCorpseLootActor, SourceLootComponent);
    DOREPLIFETIME(ABaruCorpseLootActor, bLootAvailable);
    DOREPLIFETIME(ABaruCorpseLootActor, LootRadius);
}

void ABaruCorpseLootActor::InitializeLootTarget(
    UBaruMonsterItemSpawnerComponent* Source, float Radius)
{
    if (!HasAuthority() || !IsValid(Source))
    {
        return;
    }
    SourceLootComponent = Source;
    LootRadius = FMath::IsFinite(Radius) ? FMath::Clamp(Radius, 20.0f, 200.0f) : 80.0f;
    SetLootAvailable(Source->HasLoot());
}

void ABaruCorpseLootActor::SetLootAvailable(bool bAvailable)
{
    if (!HasAuthority())
    {
        return;
    }
    bLootAvailable = bAvailable;
    OnRep_InteractionState();
    ForceNetUpdate();
}

void ABaruCorpseLootActor::OnRep_InteractionState()
{
    InteractionSphere->SetSphereRadius(LootRadius);
    InteractionSphere->SetCollisionEnabled(bLootAvailable
        ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

bool ABaruCorpseLootActor::CanInteract_Implementation(APawn* Interactor) const
{
    return bLootAvailable && IsValid(SourceLootComponent)
        && SourceLootComponent->CanLoot(Interactor);
}

FText ABaruCorpseLootActor::GetInteractPromptText_Implementation(APawn* Interactor) const
{
    return FText::FromString(TEXT("F: 시체 파밍"));
}

FGameplayTag ABaruCorpseLootActor::GetInteractionTag_Implementation() const
{
    return FBaruGameplayTags::Get().Interaction_Type_Pickup;
}

float ABaruCorpseLootActor::GetInteractionDuration_Implementation() const
{
    return 0.0f;
}

void ABaruCorpseLootActor::ExecuteInteraction_Implementation(APawn* Interactor)
{
    // 거리/시선 검증은 기존 Character의 Server_ProcessInteraction에서 수행합니다.
    if (HasAuthority() && CanInteract_Implementation(Interactor))
    {
        SourceLootComponent->LootAllOnServer(Interactor);
    }
}
