// BaruVFXLibrary.cpp

#include "Effects/VFX/BaruVFXLibrary.h"

#include "BaruLog.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

namespace
{
	bool CanSpawnLocalVFX(const UObject* WorldContextObject)
	{
		if (!IsValid(WorldContextObject) || GEngine == nullptr)
		{
			BARU_LOG(
				LogBaru,
				Warning,
				TEXT("VFX 생성 실패: WorldContext가 유효하지 않습니다.")
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
				TEXT("VFX 생성 실패: World를 찾을 수 없습니다.")
				);

			return false;
		}

		// Dedicated Server는 화면을 렌더링하지 않는다.
		return !World->IsNetMode(NM_DedicatedServer);
	}
}

UNiagaraComponent* UBaruVFXLibrary::SpawnLocalVFXAtLocation(
	const UObject* WorldContextObject,
	UNiagaraSystem* SystemTemplate,
	FVector Location,
	FRotator Rotation,
	FVector Scale,
	bool bAutoDestroy,
	bool bAutoActivate)
{
	if (!CanSpawnLocalVFX(WorldContextObject))
	{
		return nullptr;
	}

	if (!IsValid(SystemTemplate))
	{
		BARU_LOG(
			LogBaru,
			Warning,
			TEXT("VFX 생성 실패: Niagara System이 설정되지 않았습니다.")
			);

		return nullptr;
	}

	return UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		WorldContextObject,
		SystemTemplate,
		Location,
		Rotation,
		Scale,
		bAutoDestroy,
		bAutoActivate,
		ENCPoolMethod::None,
		true
		);
}

UNiagaraComponent* UBaruVFXLibrary::SpawnLocalVFXAttached(
	UNiagaraSystem* SystemTemplate,
	USceneComponent* AttachToComponent,
	FName AttachPointName,
	FVector RelativeLocation,
	FRotator RelativeRotation,
	FVector Scale,
	bool bAutoDestroy,
	bool bAutoActivate)
{
	if (!IsValid(AttachToComponent))
	{
		BARU_LOG(
			LogBaru,
			Warning,
			TEXT("부착 VFX 생성 실패: AttachToComponent가 유효하지 않습니다.")
			);

		return nullptr;
	}

	if (!CanSpawnLocalVFX(AttachToComponent))
	{
		return nullptr;
	}

	if (!IsValid(SystemTemplate))
	{
		BARU_LOG(
			LogBaru,
			Warning,
			TEXT("부착 VFX 생성 실패: Niagara System이 설정되지 않았습니다.")
			);

		return nullptr;
	}

	return UNiagaraFunctionLibrary::SpawnSystemAttached(
		SystemTemplate,
		AttachToComponent,
		AttachPointName,
		RelativeLocation,
		RelativeRotation,
		Scale,
		EAttachLocation::KeepRelativeOffset,
		bAutoDestroy,
		ENCPoolMethod::None,
		bAutoActivate,
		true
		);
}
