#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Gameplay/Equipment/DataTypes/BaruEquipmentTypes.h"   // ★[추가] EBaruEquipmentSlot
#include "BaruAnimInstance.generated.h"

class ABaruCharacter;
class UCharacterMovementComponent;

/**
 * 캐릭터 ABP 의 C++ 베이스.
 * C++ 은 "상태 값 계산"만 하고, 어떤 애니메이션을 재생할지는 ABP 가 정합니다.
 * 이렇게 나누면 애니메이터가 ABP 만 만져도 되고 C++ 빌드가 필요 없습니다.
 */
UCLASS()
class BARUGAME_API UBaruAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	// 수평 이동 속도. BS_Idle_Walk_Run 의 Speed 축에 연결합니다.
	UPROPERTY(BlueprintReadOnly, Category = "BARU|Anim|Movement")
	float GroundSpeed = 0.f;

	// -180 ~ 180. 옆걸음(스트레이프) 블렌드용.
	UPROPERTY(BlueprintReadOnly, Category = "BARU|Anim|Movement")
	float Direction = 0.f;
	
	//  앉은 상태. ABP 에서 크라우치 애니메이션 분기에 사용
	UPROPERTY(BlueprintReadOnly, Category = "BARU|Anim|Movement")
	bool bIsCrouching = false;

	//  달리는 중인지. 걷기/달리기 블렌드 구분용.
	UPROPERTY(BlueprintReadOnly, Category = "BARU|Anim|Movement")
	bool bIsSprinting = false;

	// 무기를 들고 있는지. 권총 자세로 블렌드할 때 사용
	UPROPERTY(BlueprintReadOnly, Category = "BARU|Anim|Weapon")
	bool bHasWeapon = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "BARU|Anim|Weapon")
	EBaruEquipmentSlot ActiveWeaponSlot = EBaruEquipmentSlot::None;

	UPROPERTY(BlueprintReadOnly, Category = "BARU|Anim|Movement")
	bool bIsInAir = false;

	// 입력이 들어오는 중인지. 속도만 보면 미끄러질 때도 걷는 애니가 나옵니다.
	UPROPERTY(BlueprintReadOnly, Category = "BARU|Anim|Movement")
	bool bIsAccelerating = false;

	UPROPERTY(BlueprintReadOnly, Category = "BARU|Anim|State")
	bool bIsDead = false;
	
	UPROPERTY(BlueprintReadOnly, Category = "BARU|Anim|State")
	bool bIsDBNO = false;

	// 상하 조준각. 나중에 AO_Rifle 같은 에임 오프셋에 연결합니다.
	UPROPERTY(BlueprintReadOnly, Category = "BARU|Anim|Aim")
	float AimPitch = 0.f;
	
	UPROPERTY(BlueprintReadOnly, Category = "BARU|Anim|View")
	bool bIsFirstPersonMesh = false;

private:
	UPROPERTY(Transient)
	TObjectPtr<ABaruCharacter> OwnerCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UCharacterMovementComponent> OwnerMovement;
};