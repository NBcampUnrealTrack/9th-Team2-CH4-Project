// BaruCoopDoorActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaruCoopDoorActor.generated.h"

class UStaticMeshComponent;
class ABaruCoopButtonActor;
class APawn;

UCLASS()
class BARUGAME_API ABaruCoopDoorActor : public AActor
{
    GENERATED_BODY()
    
public: 
    ABaruCoopDoorActor();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    bool CanAcceptButtonPress(const ABaruCoopButtonActor* InButton, const APawn* Interactor) const;
    void NotifyButtonPressed(ABaruCoopButtonActor* InButton, APawn* Interactor);

protected:
    UFUNCTION()
    void OnRep_IsOpen();

    void HandleButtonTimeout();

    // 클라이언트 연출 동기화를 위한 멀티캐스트 RPC
    UFUNCTION(NetMulticast, Reliable)
    void Multicast_OnFirstButtonActivated(float TimeRemaining);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_OnSyncFailed();

    UFUNCTION(BlueprintImplementableEvent, Category = "BARU|CoopDoor")
    void BP_OnDoorStateChanged(bool bOpen);

    UFUNCTION(BlueprintImplementableEvent, Category = "BARU|CoopDoor")
    void BP_OnFirstButtonActivated(float TimeRemaining);

    UFUNCTION(BlueprintImplementableEvent, Category = "BARU|CoopDoor")
    void BP_OnSyncFailed();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
    TObjectPtr<USceneComponent> RootScene;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
    TObjectPtr<UStaticMeshComponent> FrameMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
    TObjectPtr<UStaticMeshComponent> ShutterMesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|CoopDoor|Rules", meta = (ClampMin = "0.5"))
    float SyncToleranceSeconds = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|CoopDoor|Movement", meta = (ClampMin = "50.0"))
    float LiftHeight = 320.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|CoopDoor|Movement", meta = (ClampMin = "0.2"))
    float LiftSpeed = 1.5f;

    UPROPERTY(ReplicatedUsing = OnRep_IsOpen, BlueprintReadOnly, Category = "BARU|CoopDoor|State")
    bool bIsOpen = false;

private:
    FVector InitialShutterLoc;

    UPROPERTY(Transient)
    TWeakObjectPtr<ABaruCoopButtonActor> FirstPressedButton;

    UPROPERTY(Transient)
    TWeakObjectPtr<APawn> FirstInteractingPlayer;

    FTimerHandle ButtonTimeoutTimerHandle;
};