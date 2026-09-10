#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaruCoopDoorActor.generated.h"

class UStaticMeshComponent;
class ABaruCoopButtonActor;
class APawn;

UENUM(BlueprintType)
enum class EBaruCoopDoorState : uint8
{
    Stopped UMETA(DisplayName = "Stopped"),
    Opening UMETA(DisplayName = "Opening"),
    Closing UMETA(DisplayName = "Closing")
};

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

    /** 버튼이 눌렸을 때 통보 */
    void NotifyButtonPressed(ABaruCoopButtonActor* InButton, APawn* Interactor);

    /** 버튼에서 손을 뗐을 때 통보 */
    void NotifyButtonReleased(ABaruCoopButtonActor* InButton, APawn* Interactor);

protected:
    UFUNCTION()
    void OnRep_DoorState();

    UFUNCTION()
    void OnRep_ShutterLoc();

    void EvaluateDoorMovement();
    void SetDoorMovementState(EBaruCoopDoorState NewState);

    UFUNCTION(BlueprintImplementableEvent, Category = "BARU|CoopDoor")
    void BP_OnDoorMovementStateChanged(EBaruCoopDoorState NewState);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
    TObjectPtr<USceneComponent> RootScene;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
    TObjectPtr<UStaticMeshComponent> FrameMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
    TObjectPtr<UStaticMeshComponent> ShutterMesh;

    /** 셔터가 위로 올라갈 최대 높이 (cm) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|CoopDoor|Movement", meta = (ClampMin = "50.0"))
    float LiftHeight = 320.0f;

    /** 2명이 누를 때 상승 속도 (cm/s, 기본 80이면 4초 동안 완개) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|CoopDoor|Movement", meta = (ClampMin = "10.0"))
    float LiftSpeed = 80.0f;

    /** 아무도 안 누를 때 하강 속도 (cm/s) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|CoopDoor|Movement", meta = (ClampMin = "10.0"))
    float LowerSpeed = 100.0f;

    UPROPERTY(ReplicatedUsing = OnRep_DoorState, BlueprintReadOnly, Category = "BARU|CoopDoor|State")
    EBaruCoopDoorState DoorState = EBaruCoopDoorState::Stopped;

    UPROPERTY(ReplicatedUsing = OnRep_ShutterLoc)
    FVector ReplicatedShutterLoc;

private:
    FVector InitialShutterLoc;

    UPROPERTY(Transient)
    TSet<TWeakObjectPtr<ABaruCoopButtonActor>> ActiveButtons;

    UPROPERTY(Transient)
    TSet<TWeakObjectPtr<APawn>> ActiveInteractors;
};