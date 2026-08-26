// BaruWeaponBase.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaruWeaponBase.generated.h"

UCLASS()
class BARUGAME_API ABaruWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	ABaruWeaponBase();
	
	
	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	// 서버에서만 실행. 발사 처리.
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void Fire(AActor* WeaponInstigator);

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float Damage = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Weapon")
	float Range = 5000.f;

};
