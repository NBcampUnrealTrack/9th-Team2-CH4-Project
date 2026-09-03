// BaruWeaponBase.h
	//[수정] 이전엔 사거리, 데미지 등을 이곳에서 결정했는데, 지금은 서버에서 결정.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaruWeaponBase.generated.h"

class USkeletalMeshComponent;
class UBaruWeaponDataAsset;
class UGameplayEffect; // GAS 피해 GameplayEffect

UCLASS()
class BARUGAME_API ABaruWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	ABaruWeaponBase();
	
	
	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

		// 서버에서만 실행. 발사 처리. EqupmentComponent가 서버 검증을 끝낸 뒤 호출하는 실제 발사.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Weapon")
	virtual void Fire(AActor* WeaponInstigator);
	
		// EquipmentComponent가 무기를 생성한 직후,
		// Weapon DataAsset의 값을 이 Actor의 런타임 수치로 복사하는 함수
	void InitializeFromData(const UBaruWeaponDataAsset* WeaponData);

		// DataAsset에서 받은 실제 런타임 피해량
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Replicated,
		Category = "BARU|Weapon|Runtime")
	float Damage = 0.0f;

		// DataAsset에서 받은 실제 런타임 사거리
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Replicated,
		Category = "BARU|Weapon|Runtime")
	float Range = 0.0f;

		// DataAsset에서 받은 한 탄창 최대 탄환 수
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Replicated,
		Category = "BARU|Weapon|Runtime")
	int32 MagazineCapacity = 0;

		// DataAsset에서 받은 발사 간격(초)
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Replicated,
		Category = "BARU|Weapon|Runtime")
	float FireInterval = 0.0f;
	
		//GAS 부분.
		// DataAsset에서 불러온 피해 GameplayEffect 클래스.
		// 서버 발사 처리에서만 사용하므로 복제하지 않음.
	UPROPERTY(
		Transient,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "BARU|Weapon|Runtime")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	
	
		// 위 Replicated 변수들을 실제 네트워크 복제 목록에 등록
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
		// 서버가 허용하는 다음 발사 시각.(연속 발사 요청 간격을 검사하기 위해.)
	double NextAllowedFireTime = 0.0;
	
};
