// BaruVFXLibrary.h

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "BaruVFXLibrary.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;

/**
 * BARU 프로젝트의 로컬 Niagara VFX 생성 함수 모음
 *
 * 네트워크 전송은 담당하지 않는다
 * 호출된 컴퓨터에서만 이펙트를 생성한다.
 */
UCLASS()
class BARUGAME_API UBaruVFXLibrary
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 지정한 월드 위치에 Niagara 이펙트를 생성한다.
	 *
	 * 폭발, 피격 흔적처럼 생성 위치에 남아 있는 일회성 이펙트에 사용한다.
	 */
	UFUNCTION(
		BlueprintCallable,
		BlueprintCosmetic,
		Category = "BARU|VFX",
		meta = (
			WorldContext = "WorldContextObject",
			AdvancedDisplay = "5"
			)
			)
	static UNiagaraComponent* SpawnLocalVFXAtLocation(
		const UObject* WorldContextObject,
		UNiagaraSystem* SystemTemplate,
		FVector Location,
		FRotator Rotation,
		FVector Scale,
		bool bAutoDestroy = true,
		bool bAutoActivate = true
		);

	/**
	 * SceneComponent에 Niagara 이펙트를 부착한다.
	 *
	 * 무기 총구 화염이나 캐릭터 상태 효과처럼
	 * 대상과 함께 움직이는 이펙트에 사용한다.
	 */
	UFUNCTION(
		BlueprintCallable,
		BlueprintCosmetic,
		Category = "BARU|VFX",
		meta = (
			AdvancedDisplay = "6"
			)
			)
	static UNiagaraComponent* SpawnLocalVFXAttached(
		UNiagaraSystem* SystemTemplate,
		USceneComponent* AttachToComponent,
		FName AttachPointName,
		FVector RelativeLocation,
		FRotator RelativeRotation,
		FVector Scale,
		bool bAutoDestroy = true,
		bool bAutoActivate = true
		);
};
