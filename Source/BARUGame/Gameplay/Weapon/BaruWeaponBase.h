// BaruWeaponBase.h
	//[수정] 이전엔 사거리, 데미지 등을 이곳에서 결정했는데, 지금은 서버에서 결정.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaruWeaponBase.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;	// 스켈레탈, 스태틱 메시 둘 다 쓰기 위해.
class USceneComponent;	// 무기 액터 때문에.
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
	
	
		// 무기의 외형.
	// 본인에게만 표시할 SkeletalMesh 무기 외형.
	// StaticMesh를 쓰는 무기라면 이 메시 에셋은 비워둡니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		Category = "BARU|Weapon|FirstPerson")
	TObjectPtr<USkeletalMeshComponent> FirstPersonWeaponMesh;

	// 본인에게만 표시할 StaticMesh 무기 외형.
	// SkeletalMesh를 쓰는 무기라면 이 메시 에셋은 비워둡니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		Category = "BARU|Weapon|FirstPerson")
	TObjectPtr<UStaticMeshComponent> FirstPersonStaticMesh;

	// 서버가 활성/비활성 전환 시 본인용 외형의 표시 상태를 설정.
	void SetFirstPersonWeaponActive(bool bNewActive);

	// Owner가 클라이언트에 복제되면 팔 부착을 다시 확인.
	virtual void OnRep_Owner() override;

	// 현재 화면에 맞는 총구 효과 부착 메시를 반환.
	// 본인 화면은 1인칭, 타인 화면은 3인칭 메시를 사용.
	UFUNCTION(BlueprintPure, Category = "BARU|Weapon|Effects")
	USceneComponent* GetFireEffectAttachComponent() const;
	
	// 발사 관련 서버 부분.
		// 서버에서만 실행. 발사 처리. EqupmentComponent가 서버 검증을 끝낸 뒤 호출하는 실제 발사.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Weapon")
	virtual void Fire(AActor* WeaponInstigator);
	
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
	
		// 외형 추가를 위해.
		// 복제하는 것은 무기 선택 상태와 설정값.
protected:
	virtual void BeginPlay() override;
	
	// 서버에서 DA 값을 받아 클라이언트에도 전달.
	// 각 클라이언트는 자신의 팔 메시에 외형을 부착합니다.
	UPROPERTY(Transient, ReplicatedUsing = OnRep_FirstPersonState)
	FName FirstPersonHandSocketName = TEXT("hand_r");

	UPROPERTY(Transient, ReplicatedUsing = OnRep_FirstPersonState)
	FTransform FirstPersonHandRelativeTransform = FTransform::Identity;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_FirstPersonState)
	bool bFirstPersonWeaponActive = false;

	UFUNCTION()
	void OnRep_FirstPersonState();

	void RefreshFirstPersonVisual();
};
