#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaruTensionComponent.generated.h"

class UAudioComponent;
class USoundBase;
class ABaruPlayerState;

/**
 * 긴장도(Tension) 수치에 따라 로컬 심장소리 연출 및 서버 근접 연산을 전담하는 컴포넌트
 */
UCLASS(ClassGroup = (BARU), meta = (BlueprintSpawnableComponent))
class BARUGAME_API UBaruTensionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBaruTensionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// [Client] PlayerState 바인딩 시도 (로컬 클라이언트 복제 타이밍 대응)
	void TryBindToPlayerState();

	// [Client] 긴장도 수치 변경 수신 콜백
	UFUNCTION()
	void HandleTensionChanged(float NewTension);

	// [Server] 주기적 몬스터 근접 체크 및 Tension 가감
	void UpdateTensionOnServer();

protected:
	// =========================================================================
	// 오디오 연출 설정 (클라이언트 전용)
	// =========================================================================
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Tension|Audio")
	TObjectPtr<USoundBase> HeartbeatSound;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Tension|Audio")
	TObjectPtr<UAudioComponent> HeartbeatAudioComponent;

	// 심장소리가 들리기 시작하는 최소 긴장도 기준 (기본: 15.0)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Tension|Audio", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float SoundThresholdTension = 15.0f;

	// 심장소리 볼륨 보간 속도
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Tension|Audio")
	float AudioInterpSpeed = 3.0f;

	// 최대 심장소리 볼륨 및 피치(심박 속도)
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Tension|Audio")
	float MaxHeartbeatVolume = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "BARU|Tension|Audio")
	float MinHeartbeatPitch = 0.9f;

	UPROPERTY(EditDefaultsOnly, Category = "BARU|Tension|Audio")
	float MaxHeartbeatPitch = 1.45f;

	// =========================================================================
	// 서버 근접 연산 설정 (서버 전용)
	// =========================================================================
	// 몬스터 감지 반경 (cm)
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Tension|Proximity")
	float ProximityRadius = 1200.0f;

	// 초당 긴장도 증가량 (가장 가까울 때)
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Tension|Proximity")
	float MaxTensionGainRate = 12.0f;

	// 몬스터가 없을 때 초당 자연 감소량
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Tension|Proximity")
	float TensionDecayRate = 6.0f;

	// 서버 근접 검사 주기 (초 단위, 0.25초)
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Tension|Proximity")
	float ServerCheckInterval = 0.25f;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<ABaruPlayerState> CachedPlayerState;

	FTimerHandle ServerCheckTimerHandle;
	FTimerHandle ClientBindRetryTimerHandle;

	float TargetVolume = 0.0f;
	float TargetPitch = 1.0f;
	float CurrentVolume = 0.0f;
	float CurrentPitch = 1.0f;
};