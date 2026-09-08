// BaruSlidingDoorActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/InteractableInterface.h"
#include "BaruSlidingDoorActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class BARUGAME_API ABaruSlidingDoorActor : public AActor, public IInteractableInterface
{
    GENERATED_BODY()
    
public: 
    ABaruSlidingDoorActor();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    virtual bool CanInteract_Implementation(APawn* Interactor) const override;
    virtual FText GetInteractPromptText_Implementation(APawn* Interactor) const override;
    virtual FGameplayTag GetInteractionTag_Implementation() const override;
    virtual float GetInteractionDuration_Implementation() const override;
    virtual void ExecuteInteraction_Implementation(APawn* Interactor) override;

protected:
    UFUNCTION()
    void OnRep_IsOpen();

    UFUNCTION(BlueprintImplementableEvent, Category = "BARU|SlidingDoor")
    void BP_OnDoorStateChanged(bool bOpen);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
    TObjectPtr<USceneComponent> RootScene;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
    TObjectPtr<UStaticMeshComponent> DoorFrameMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
    TObjectPtr<UStaticMeshComponent> LeftDoorMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
    TObjectPtr<UStaticMeshComponent> RightDoorMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Door|Movement", meta = (ClampMin = "10.0"))
    float SlideDistance = 110.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Door|Movement", meta = (ClampMin = "0.5"))
    float SlideSpeed = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Door|Rules")
    bool bCanToggleClose = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Door|Rules")
    bool bIsLocked = false;

    UPROPERTY(ReplicatedUsing = OnRep_IsOpen, BlueprintReadOnly, Category = "BARU|Door|State")
    bool bIsOpen = false;

private:
    FVector InitialLeftDoorLoc;
    FVector InitialRightDoorLoc;
    bool bIsMoving = false;
};