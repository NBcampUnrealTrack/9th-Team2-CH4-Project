#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/InteractableInterface.h"
#include "BaruCorpseLootActor.generated.h"

class UBaruMonsterItemSpawnerComponent;
class USphereComponent;

// 외형 없는 시체 상호작용 대상. 바닥 Pickup이나 새 몬스터 Actor가 아닙니다.
UCLASS()
class BARUGAME_API ABaruCorpseLootActor : public AActor, public IInteractableInterface
{
    GENERATED_BODY()

public:
    ABaruCorpseLootActor();
    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void InitializeLootTarget(UBaruMonsterItemSpawnerComponent* Source, float Radius);
    void SetLootAvailable(bool bAvailable);

    virtual bool CanInteract_Implementation(APawn* Interactor) const override;
    virtual FText GetInteractPromptText_Implementation(APawn* Interactor) const override;
    virtual FGameplayTag GetInteractionTag_Implementation() const override;
    virtual float GetInteractionDuration_Implementation() const override;
    virtual void ExecuteInteraction_Implementation(APawn* Interactor) override;

protected:
    UPROPERTY(VisibleAnywhere, Category = "BARU|Corpse Loot")
    TObjectPtr<USphereComponent> InteractionSphere;

private:
    UPROPERTY(Replicated)
    TObjectPtr<UBaruMonsterItemSpawnerComponent> SourceLootComponent;

    UPROPERTY(ReplicatedUsing = OnRep_InteractionState)
    bool bLootAvailable = false;

    UPROPERTY(ReplicatedUsing = OnRep_InteractionState)
    float LootRadius = 80.0f;

    UFUNCTION()
    void OnRep_InteractionState();
};
