// BaruAudioLibrary.cpp

#include "Effects/Audio/BaruAudioLibrary.h"

#include "BaruLog.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
	bool CanPlayLocalAudio(const UObject* WorldContextObject)
	{
		if (!IsValid(WorldContextObject) || GEngine == nullptr)
		{
			BARU_LOG(
				LogBaru,
				Warning,
				TEXT("오디오 재생 실패: WorldContext가 유효하지 않습니다.")
				);

			return false;
		}

		const UWorld* World =
			GEngine->GetWorldFromContextObject(
				WorldContextObject,
				EGetWorldErrorMode::ReturnNull
				);
		if (!IsValid(World))
		{
			BARU_LOG(
				LogBaru,
				Warning,
				TEXT("오디오 재생 실패: World를 찾을 수 없습니다.")
				);

			return false;
		}

		// Dedicated Server에는 소리를 출력할 장치가 없다.
		return !World->IsNetMode(NM_DedicatedServer);
	}
}

void UBaruAudioLibrary::PlayLocalSound2D(
	const UObject* WorldContextObject,
	USoundBase* Sound,
	float VolumeMultiplier,
	float PitchMultiplier,
	float StartTime,
	USoundConcurrency* ConcurrencySettings,
	const AActor* OwningActor,
	bool bIsUISound)
{
	if (!CanPlayLocalAudio(WorldContextObject))
	{
		return;
	}

	if (!IsValid(Sound))
	{
		BARU_LOG(
			LogBaru,
			Warning,
			TEXT("2D 오디오 재생 실패: Sound가 설정되지 않았습니다.")
			);

		return;
	}

	UGameplayStatics::PlaySound2D(
		WorldContextObject,
		Sound,
		VolumeMultiplier,
		PitchMultiplier,
		StartTime,
		ConcurrencySettings,
		OwningActor,
		bIsUISound
		);
}

UAudioComponent* UBaruAudioLibrary::SpawnLocalSoundAtLocation(
	const UObject* WorldContextObject,
	USoundBase* Sound,
	FVector Location,
	FRotator Rotation,
	float VolumeMultiplier,
	float PitchMultiplier,
	float StartTime,
	USoundAttenuation* AttenuationSettings,
	USoundConcurrency* ConcurrencySettings,
	bool bAutoDestroy)
{
	if (!CanPlayLocalAudio(WorldContextObject))
	{
		return nullptr;
	}

	if (!IsValid(Sound))
	{
		BARU_LOG(
			LogBaru,
			Warning,
			TEXT("3D 오디오 재생 실패: Sound가 설정되지 않았습니다.")
			);

		return nullptr;
	}

	return UGameplayStatics::SpawnSoundAtLocation(
		WorldContextObject,
		Sound,
		Location,
		Rotation,
		VolumeMultiplier,
		PitchMultiplier,
		StartTime,
		AttenuationSettings,
		ConcurrencySettings,
		bAutoDestroy
		);
}
