// BaruMapTransitionVolume.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UObject/SoftObjectPtr.h"
#include "BaruMapTransitionVolume.generated.h"

class UBoxComponent;

UCLASS()
class BARUGAME_API ABaruMapTransitionVolume : public AActor
{
	GENERATED_BODY()
    
public: 
	ABaruMapTransitionVolume();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleVolumeBeginOverlap(
	   UPrimitiveComponent* OverlappedComponent,
	   AActor* OtherActor,
	   UPrimitiveComponent* OtherComp,
	   int32 OtherBodyIndex,
	   bool bFromSweep,
	   const FHitResult& SweepResult
	);

	UFUNCTION()
	void HandleVolumeEndOverlap(
	   UPrimitiveComponent* OverlappedComponent,
	   AActor* OtherActor,
	   UPrimitiveComponent* OtherComp,
	   int32 OtherBodyIndex
	);

	void ExecuteTransition();
	bool CheckAllPlayersInVolume() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
	TObjectPtr<UBoxComponent> TransitionTrigger;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Transition")
	TSoftObjectPtr<UWorld> TargetLevel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Transition")
	bool bRequireAllAlivePlayers = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Transition", meta = (ClampMin = "0.0"))
	float TransitionDelay = 1.0f;

	UFUNCTION(BlueprintImplementableEvent, Category = "BARU|Transition")
	void BP_OnTransitionTriggered();

	UFUNCTION(BlueprintImplementableEvent, Category = "BARU|Transition")
	void BP_OnTransitionCancelled();

private:
	bool bHasTriggered = false;
	TSet<TWeakObjectPtr<APawn>> OverlappedPlayers;
	FTimerHandle TransitionTimerHandle;
};