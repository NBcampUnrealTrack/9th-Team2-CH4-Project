//BaruWeaponDataAsset.h
/*
 무기마다 다른 정적 설정을 보관하는 DataAsset 클래스.
 
 실제 에셋 예시:
 - DA_Weapon_Revolver
 - DA_Weapon_Rifle
 
 아이템 이름, 아이콘, GridSize 등 공통 정보는 DT_Item이 관리.
 이 클래스는 무기만의 상세 정보만 관리.
 */


#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

	// 앞에서 만든 장비 슬롯 Enum 사용
#include "Gameplay/Equipment/DataTypes/BaruEquipmentTypes.h"

#include "BaruWeaponDataAsset.generated.h"

	// 포인터로만 사용하기 때문에 전방 선언
class ABaruWeaponBase;
class UGameplayAbility;	// GA용.


UCLASS(BlueprintType)
class BARUGAME_API UBaruWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
		// 이 무기가 기본적으로 들어갈 장착 슬롯
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Weapon")
	EBaruEquipmentSlot EquipmentSlot = EBaruEquipmentSlot::None;
	// |				|				ㄴ 시작값 : 아직 슬롯 미지정.
	// |				ㄴ 변수 이름 : 이 무기의 장착 위치
	// ㄴ 변수 자료형 : 장착 슬롯 Enum만 넣기 가능
	

		// 장착 시 월드에 생성할 무기 Actor의 Blueprint 클래스
		// 예: BP_Weapon_Revolver, BP_Weapon_Rifle
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Weapon")
	TSoftClassPtr<ABaruWeaponBase> WeaponActorClass;
	
		// 이 무기를 활성화했을 때 ASC에 부여할 발사 GA
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Weapon|Abilities")
	TSubclassOf<UGameplayAbility> FireAbilityClass;
	
	//-----하단 두 함수 : 비활성 무기 장착 슬롯 부분. 허리랑 등짝.
		// 이 무기가 비활성 상태일 때 붙을 Character Mesh 소켓 이름.
		// 예: Weapon_BackSocket, Weapon_HipSocket
		// 나중에 부착할 부분의 이름을 그렇게 정하면 됨.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "BARU|Weapon|Attachment")
	FName HolsterSocketName = NAME_None;
	
		// 홀스터 소켓에 붙은 뒤 적용할 무기별 위치·회전 보정값.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "BARU|Weapon|Attachment")
	FTransform HolsterRelativeTransform = FTransform::Identity;
	
		// 손 소켓에 부착한 뒤 적용할 무기별 위치·회전·크기 보정값.
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "BARU|Weapon|Attachment")
	FTransform HandRelativeTransform = FTransform::Identity;
	


		// 한 탄창에 들어가는 최대 탄환 수
	UPROPERTY(EditDefaultsOnly,	BlueprintReadOnly,	Category = "BARU|Weapon|Ammo",
		meta = (ClampMin = "1"))
	int32 MagazineCapacity = 6;
	
	
	// [09.13] 재장전 Ability 클래스 추가
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Weapon|Abilities")
	TSubclassOf<UGameplayAbility> ReloadAbilityClass;
};