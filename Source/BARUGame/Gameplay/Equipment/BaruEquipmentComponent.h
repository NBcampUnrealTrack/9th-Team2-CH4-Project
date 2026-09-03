// BaruEquipmentComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Gameplay/Equipment/DataTypes/BaruEquipmentTypes.h"
#include "BaruEquipmentComponent.generated.h"

class ABaruWeaponBase;
class UBaruWeaponDataAsset;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BARUGAME_API UBaruEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBaruEquipmentComponent();

		// 네트워크로 복제할 변수 목록을 등록하는 함수
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
		// 서버에서 Weapon DataAsset을 받아 실제 Weapon Actor를 생성·장착.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "BARU|Equipment|Weapon")
	bool EquipWeapon(UBaruWeaponDataAsset* WeaponData);

		// 지정한 무기 슬롯의 Weapon Actor를 제거.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "BARU|Equipment|Weapon")
	void UnequipWeapon(EBaruEquipmentSlot WeaponSlot);

protected:
		// 주무기 슬롯에 실제로 생성되어 있는 Weapon Actor
		// 아직 Equip 함수가 없으므로 현재는 비어 있는 상태.
	UPROPERTY(
		Transient,
		Replicated,
		BlueprintReadOnly,
		Category = "BARU|Equipment|Weapon")
	TObjectPtr<ABaruWeaponBase> PrimaryWeapon;

		// 보조무기 슬롯에 실제로 생성되어 있는 Weapon Actor
	UPROPERTY(
		Transient,
		Replicated,
		BlueprintReadOnly,
		Category = "BARU|Equipment|Weapon")
	TObjectPtr<ABaruWeaponBase> SecondaryWeapon;

		// 현재 손에 들고 사용 중인 무기 슬롯
		// 지금은 None이며, 다음 단계에서 PrimaryWeapon 또는 SecondaryWeapon으로 변경.
	UPROPERTY(
		Replicated,
		BlueprintReadOnly,
		Category = "BARU|Equipment|Weapon")
	EBaruEquipmentSlot ActiveWeaponSlot = EBaruEquipmentSlot::None;
	
		// 3인칭 Character Mesh에서 무기를 붙일 소켓 또는 본 이름
		// 현재 Character Skeleton의 weapon_r을 기본값으로 사용.
		// 나중에 캐릭터 변경 시, 확인 필요한, 바꿔야할 부분!!!
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Equipment|Weapon")
	FName ThirdPersonWeaponAttachPoint = TEXT("weapon_r");
	
	
};