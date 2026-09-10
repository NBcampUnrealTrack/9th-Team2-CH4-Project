#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaruSpawnerControlVolume.generated.h"

class UBoxComponent;

UENUM(BlueprintType)
enum class EBaruSpawnerControlAction : uint8
{
	StopSpawning        UMETA(DisplayName = "Stop Spawning"),
	ResumeSpawning      UMETA(DisplayName = "Resume Spawning"),
	DestroySpawners     UMETA(DisplayName = "Destroy Spawners")
};

UCLASS()
class BARUGAME_API ABaruSpawnerControlVolume : public AActor
{
	GENERATED_BODY()

public:
	ABaruSpawnerControlVolume();

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

	void ProcessSpawnerControl();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Components")
	TObjectPtr<UBoxComponent> TriggerVolume;

	// 제어 대상 스포너 액터 목록 (레벨 에디터의 스포이드로 직접 지정)
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "BARU|SpawnerControl")
	TArray<TObjectPtr<AActor>> TargetSpawners;

	// 특정 액터 태그를 가진 스포너들을 월드에서 찾아 함께 정지시킬지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|SpawnerControl")
	FName TargetSpawnerTag = NAME_None;

	// 플레이어 진입 시 취할 동작
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|SpawnerControl")
	EBaruSpawnerControlAction ControlAction = EBaruSpawnerControlAction::StopSpawning;

	// 1회만 발동하고 볼륨을 비활성화할지 여부
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|SpawnerControl")
	bool bTriggerOnce = true;

	// 블루프린트 연출/사운드용 훅
	UFUNCTION(BlueprintImplementableEvent, Category = "BARU|SpawnerControl")
	void BP_OnSpawnersControlled(EBaruSpawnerControlAction ActionTaken, int32 AffectedCount);

private:
	bool bHasTriggered = false;
};