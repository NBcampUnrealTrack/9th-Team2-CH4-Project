//BaruEquipmentTypes.h
// UPrimaryDataAsset으로 생성. UDataAsset : 단순 설정 파일 vs. UPrimaryDataAsset : 게임에서 식별, 관리, 로드할 중요한 정의 파일.

#pragma once

#include "CoreMinimal.h"
#include "BaruEquipmentTypes.generated.h"

// 아이템이 장착될 수 있는 슬롯 종류
UENUM(BlueprintType)
enum class EBaruEquipmentSlot : uint8
{
	None        UMETA(DisplayName = "None"),

		// 현재 먼저 구현할 무기 슬롯
	PrimaryWeapon     UMETA(DisplayName = "Primary Weapon"),
	SecondaryWeapon   UMETA(DisplayName = "Secondary Weapon"),

		// 추후 구현할 슬롯
	Head        UMETA(DisplayName = "Head Equipment"),
	Body        UMETA(DisplayName = "Body Equipment"),
	
		// 추가 가방을 실제로 장착하는 슬롯. 만약 나중에 인벤 격자 증가나최대 무게 증가, 개인/공용 가방 접근 등 하려면.
	Back   UMETA(DisplayName = "Back")
};

	// 장비창과 MainHUD에서 함께 표시하는 즉시 사용 슬롯.
	// 소비템은 장비창에서도 보이고 다른 곳에서도 보일 수 있도록.
UENUM(BlueprintType)
enum class EBaruQuickSlot : uint8
{
	None UMETA(DisplayName = "None"),

		// 수류탄 등 투척 아이템
	Throwable UMETA(DisplayName = "Throwable"),

		// 체력 회복 소비 아이템
	HealthRecovery UMETA(DisplayName = "Health Recovery"),

		// 정신력 회복 소비 아이템
	SanityRecovery UMETA(DisplayName = "Sanity Recovery")
};

