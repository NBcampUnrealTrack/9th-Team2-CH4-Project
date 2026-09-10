// BaruWeaponBase.h
	//[수정] 이전엔 사거리, 데미지 등을 이곳에서 결정했는데, 지금은 서버에서 결정.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaruWeaponBase.generated.h"

class USkeletalMeshComponent;
class USceneComponent;	// 무기 액터 때문에.
class UBaruWeaponDataAsset;


UCLASS()
class BARUGAME_API ABaruWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	ABaruWeaponBase();
	
	
	UPROPERTY(VisibleAnywhere, Category = "Weapon")
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;
	
	


	// 현재 화면에 맞는 총구 효과 부착 메시를 반환.
	UFUNCTION(BlueprintPure, Category = "BARU|Weapon|Effects")
	USceneComponent* GetFireEffectAttachComponent() const;
	
	
		// EquipmentComponent가 무기를 생성한 직후,
		// Weapon DataAsset의 값을 이 Actor의 런타임 수치로 복사하는 함수
	void InitializeFromData(const UBaruWeaponDataAsset* WeaponData);
	
	//---------Holster(아래 두 함수)까지 비활성 무기 장착 부분.
		// EquipmentComponent가 무기를 홀스터 위치로 옮길 때 사용.
	FName GetHolsterSocketName() const
	{
		return HolsterSocketName;
	}

		const FTransform& GetHolsterRelativeTransform() const
	{
		return HolsterRelativeTransform;
	}
	
		//위치, 방향 등.
	const FTransform& GetHandRelativeTransform() const
	{
		return HandRelativeTransform;
	}

		// DataAsset에서 받은 한 탄창 최대 탄환 수
	UPROPERTY(
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Replicated,
		Category = "BARU|Weapon|Runtime")
	int32 MagazineCapacity = 0;
	
		//매시용 bool 프로퍼티.
	UPROPERTY(
        EditDefaultsOnly,
        BlueprintReadOnly,
        Category = "BARU|Weapon|Visual")
    bool bHideFromOwner = false;
	
	
	//-----비활성 무기 장착 부분/
		// 비활성 상태일 때 붙을 Character Mesh 소켓.
		// DataAsset에서 장착 시 한 번 복사받습니다.
	UPROPERTY(Transient, VisibleInstanceOnly,
		BlueprintReadOnly, Category = "BARU|Weapon|Attachment")
	FName HolsterSocketName = NAME_None;

		// 홀스터 소켓에 부착한 뒤 적용할 무기별 위치·회전 보정값.
	UPROPERTY(Transient, VisibleInstanceOnly,
		BlueprintReadOnly, Category = "BARU|Weapon|Attachment")
	FTransform HolsterRelativeTransform = FTransform::Identity;
	
	// DataAsset에서 복사된 손 장착 보정값.
	UPROPERTY(
		Transient,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "BARU|Weapon|Attachment")
	FTransform HandRelativeTransform = FTransform::Identity;
	
	
		// 위 Replicated 변수들을 실제 네트워크 복제 목록에 등록
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	
		// 외형 추가를 위해.
		// 복제하는 것은 무기 선택 상태와 설정값.
protected:
	virtual void BeginPlay() override;
	


};
