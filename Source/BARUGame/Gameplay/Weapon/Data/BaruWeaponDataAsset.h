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
class UGameplayEffect;	//GAS 용.
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
	
		//팔ㅇㅔ 무기 부착(09.09.)
	// 본인용 팔 메시에서 무기를 붙일 본 또는 소켓 이름.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "BARU|Weapon|FirstPerson")
	FName FirstPersonHandSocketName = TEXT("hand_r");

	// 본인용 팔에 붙은 무기의 위치·회전·크기 보정값.
	// 기존 HandRelativeTransform은 타인에게 보이는 전신용.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "BARU|Weapon|FirstPerson")
	FTransform FirstPersonHandRelativeTransform = FTransform::Identity;


		// 무기의 기본 피해량
		// 추후 GAS GameplayEffect에 전달할 기본 수치로 사용
	UPROPERTY(EditDefaultsOnly,	BlueprintReadOnly, Category = "BARU|Weapon|Combat",	meta = (ClampMin = "0.0"))
	float BaseDamage = 10.0f;


		// 무기의 최대 유효 사거리
		// Unreal의 기본 거리 단위는 cm
	UPROPERTY(EditDefaultsOnly,	BlueprintReadOnly, Category = "BARU|Weapon|Combat",
													meta = (ClampMin = "0.0", Units = "cm"))
	float MaxRange = 5000.0f;


		// 한 탄창에 들어가는 최대 탄환 수
	UPROPERTY(EditDefaultsOnly,	BlueprintReadOnly,	Category = "BARU|Weapon|Ammo",
		meta = (ClampMin = "1"))
	int32 MagazineCapacity = 6;


		// 한 발 발사 후 다음 발까지 기다리는 시간. s는 초.
	UPROPERTY(EditDefaultsOnly,	BlueprintReadOnly,	Category = "BARU|Weapon|Combat",
		meta = (ClampMin = "0.01", Units = "s"))
	float FireInterval = 0.25f;
	
	
	// GAS.
		// 이 무기가 명중했을 때 적용할 GAS GameplayEffect.
		// 실제 피해량은 BaseDamage를 Data.Damage로 전달.
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "BARU|Weapon|GAS")
	TSoftClassPtr<UGameplayEffect> DamageEffectClass;
};