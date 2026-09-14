#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/InteractableInterface.h"
#include "BaruCoopButtonActor.generated.h"

class UStaticMeshComponent;
class ABaruCoopDoorActor;
class USoundBase;
class USoundAttenuation;

/**
 * 2인 협동 문에 신호를 보내는 스위치 액터
 * IInteractableInterface를 통해 F키 상호작용을 처리합니다.
 */
UCLASS()
class BARUGAME_API ABaruCoopButtonActor : public AActor, public IInteractableInterface
{
	GENERATED_BODY()
    
public: 
	ABaruCoopButtonActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IInteractableInterface
	virtual bool CanInteract_Implementation(APawn* Interactor) const override;
	virtual FText GetInteractPromptText_Implementation(APawn* Interactor) const override;
	virtual FGameplayTag GetInteractionTag_Implementation() const override;
	virtual float GetInteractionDuration_Implementation() const override;
	virtual void ExecuteInteraction_Implementation(APawn* Interactor) override;
	virtual void EndInteraction_Implementation(APawn* Interactor) override;
	void SetButtonActive(bool bActive);

protected:
	UFUNCTION()
	void OnRep_IsPressed();

	UFUNCTION(BlueprintImplementableEvent, Category = "BARU|CoopButton")
	void BP_OnButtonPressedStateChanged(bool bPressed);
	
	void CheckHoldingPlayerValidity();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
	TObjectPtr<UStaticMeshComponent> ButtonMesh;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "BARU|CoopDoor")
	TObjectPtr<ABaruCoopDoorActor> TargetDoor;

	UPROPERTY(ReplicatedUsing = OnRep_IsPressed, BlueprintReadOnly, Category = "BARU|CoopDoor")
	bool bIsPressed = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|CoopDoor")
	float MaxHoldDistance = 250.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|CoopButton|Audio")
	TObjectPtr<USoundBase> ButtonPressSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|CoopButton|Audio")
	TObjectPtr<USoundBase> ButtonReleaseSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|CoopButton|Audio")
	TObjectPtr<USoundAttenuation> SoundAttenuation;
	
private:
	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> HoldingPlayer;

	FTimerHandle HoldCheckTimerHandle;
};