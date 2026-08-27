// BaruAudioLibrary.h

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "BaruAudioLibrary.generated.h"

class AActor;
class UAudioComponent;
class USoundAttenuation;
class USoundBase;
class USoundConcurrency;

/**
 * Baru 프로젝트의 로컬 오디오 재생 함수 모음
 *
 * 네트워크 전송은 담당하지 않는다
 * 호출된 컴퓨터에서만 사운드를 재생한다.
 */
UCLASS()
class BARUGAME_API UBaruAudioLibrary
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 위치와 거리 감쇠가 없는 2D 사운드를 재생한다.
	 * UI 버튼음, 체력 경고음 등에 사용한다.
	 */
	UFUNCTION(
		BlueprintCallable,
		BlueprintCosmetic,
		Category = "BARU|Audio",
		meta = (
			WorldContext = "WorldContextObject",
			AdvancedDisplay = "4"
			)
			)
	static void PlayLocalSound2D(
		const UObject* WorldContextObject,
		USoundBase* Sound,
		float VolumeMultiplier = 1.0f,
		float PitchMultiplier = 1.0f,
		float StartTime = 0.0f,
		USoundConcurrency* ConcurrencySettings = nullptr,
		const AActor* OwningActor = nullptr,
		bool bIsUISound = true
		);

	/**
	 * 지정한 월드 위치에서 3D 사운드를 재생한다.
	 *
	 * 반환된 AudioComponent로 재생 중지나 볼륨 변경을 할 수 있다.
	 */
	UFUNCTION(
		BlueprintCallable,
		BlueprintCosmetic,
		Category = "BARU|Audio",
		meta = (
			WorldContext = "WorldContextObject",
			AdvancedDisplay = "4"
			)
			)
	static UAudioComponent* SpawnLocalSoundAtLocation(
		const UObject* WorldContextObject,
		USoundBase* Sound,
		FVector Location,
		FRotator Rotation,
		float VolumeMultiplier = 1.0f,
		float PitchMultiplier = 1.0f,
		float StartTime = 0.0f,
		USoundAttenuation* AttenuationSettings = nullptr,
		USoundConcurrency* ConcurrencySettings = nullptr,
		bool bAutoDestroy = true
		);
};
