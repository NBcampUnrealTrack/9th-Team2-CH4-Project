// BaruEquipmentComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Gameplay/Equipment/DataTypes/BaruEquipmentTypes.h"
#include "GameplayAbilitySpec.h"
#include "BaruEquipmentComponent.generated.h"

class ABaruWeaponBase;
class UBaruWeaponDataAsset;
class UGameplayAbility;
class UBaruAbilitySystemComponent;
class UBaruItemInstance;
class UBaruInventoryComponent;


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBaruEquipmentUpdated);

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
	bool EquipWeapon(
		UBaruWeaponDataAsset* WeaponData,
		UBaruItemInstance* SourceItem);
	
		// 지정한 무기 슬롯의 Weapon Actor를 제거.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "BARU|Equipment|Weapon")
	void UnequipWeapon(EBaruEquipmentSlot WeaponSlot);
	
	// 로컬 입력 또는 UI에서 호출하는 장비 해제 요청.
	UFUNCTION(BlueprintCallable, Category = "BARU|Equipment|Weapon")
	void RequestUnequipWeapon(EBaruEquipmentSlot WeaponSlot);
	
		// GAS.
	// ActiveWeaponSlot에 따라 현재 손에 든 무기를 반환.
	UFUNCTION(BlueprintPure, Category = "BARU|Equipment")
	ABaruWeaponBase* GetActiveWeapon() const;
	
		// 장착 상태를 ASC의 발사 GA에 반영.
		// Character의 ASC 초기화가 완료된 시점에도 호출.
	void SyncActiveWeaponFireAbilityOnServer();

		// GAS.
	// Character 입력을 현재 장착 무기에 전달.
	UFUNCTION(BlueprintCallable, Category = "BARU|Equipment")
	void RequestFireActiveWeapon();
	
		// 발사 버튼을 놓았을 때 호출. GA 연결.
	UFUNCTION(BlueprintCallable, Category = "BARU|Equipment")
	void RequestStopFireActiveWeapon();
	
/* GA 발사로 전환하여 기존 WeaponBase 직접 발사 경로가 불필요해짐.
 *	// 서버 RPC를 추가.
		// 클라이언트의 발사 입력을 서버로 전달.
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestFireActiveWeapon();
*/
	
		// 지정한 무기 슬롯에 장착된 인벤토리 아이템을 반환.
		// UI 아이콘과 이름 조회에 사용.
	UFUNCTION(BlueprintPure, Category = "BARU|Equipment|Weapon")
	UBaruItemInstance* GetEquippedWeaponItem(
		EBaruEquipmentSlot WeaponSlot) const;

		// 장착·해제·활성 무기 전환 시 UI에 변경을 알림
	UPROPERTY(BlueprintAssignable, Category = "BARU|Equipment")
	FOnBaruEquipmentUpdated OnEquipmentUpdated;

protected:
	UBaruInventoryComponent*
	GetOwnerInventoryComponent() const;
	
	// 서버가 현재 부여한 무기 발사 GA를 추적합니다.
	// 무기 전환 시 기존 GA를 제거하기 위해 필요합니다.
	// 이 장비 컴포넌트가 부여한 발사 GA만 제거합니다.
	void ClearActiveWeaponFireAbilityOnServer();
	
	// 현재 부여한 발사 GA의 식별자.
	FGameplayAbilitySpecHandle ActiveFireAbilityHandle;

	// 기존 GA가 어느 ASC에 부여되었는지 추적합니다.
	// PlayerState 교체나 캐릭터 제거 시 안전하게 정리하기 위한 약한 참조입니다.
	// 캐릭터와 PlayerState의 연결이 끊겨도 기존 GA를 정리하기 위해 보관합니다.
	TWeakObjectPtr<UBaruAbilitySystemComponent> ActiveFireAbilityASC;
	
	
		// 주무기 슬롯에 실제로 생성되어 있는 Weapon Actor
		// 아직 Equip 함수가 없으므로 현재는 비어 있는 상태.
	UPROPERTY(Transient,Replicated,	BlueprintReadOnly,Category = "BARU|Equipment|Weapon")
	TObjectPtr<ABaruWeaponBase> PrimaryWeapon;
	
	// 주무기 Actor를 생성할 때 사용한 인벤토리 아이템입니다.
	UPROPERTY(Transient,ReplicatedUsing = OnRep_EquipmentState)
	TObjectPtr<UBaruItemInstance> PrimaryWeaponItem;
	
	
		// 각 무기가 사용할 Fire GA. 서버에서 장착 시 DA로부터 기록.
	UPROPERTY(Transient)
	TSubclassOf<UGameplayAbility> PrimaryFireAbilityClass;

		// 보조무기 슬롯에 실제로 생성되어 있는 Weapon Actor
	UPROPERTY(Transient,Replicated,	BlueprintReadOnly,	Category = "BARU|Equipment|Weapon")
	TObjectPtr<ABaruWeaponBase> SecondaryWeapon;
	
	// 보조무기 Actor를 생성할 때 사용한 인벤토리 아이템입니다.
	UPROPERTY(
		Transient,
		ReplicatedUsing = OnRep_EquipmentState)
	TObjectPtr<UBaruItemInstance> SecondaryWeaponItem;
	
		// 각 무기가 사용할 Fire GA. 서버에서 장착 시 DA로부터 기록 - 2.
	UPROPERTY(Transient)
	TSubclassOf<UGameplayAbility> SecondaryFireAbilityClass;
	
		// 현재 손에 들고 사용 중인 무기 슬롯
		// 지금은 None이며, 다음 단계에서 PrimaryWeapon 또는 SecondaryWeapon으로 변경.
	UPROPERTY(	ReplicatedUsing = OnRep_EquipmentState,	BlueprintReadOnly,	Category = "BARU|Equipment|Weapon")
	EBaruEquipmentSlot ActiveWeaponSlot = EBaruEquipmentSlot::None;
	
		// 3인칭 Character Mesh에서 무기를 붙일 소켓 또는 본 이름
		// 현재 Character Skeleton의 hand_r을 기본값으로 사용.
		// 나중에 캐릭터 변경 시, 확인 필요한, 바꿔야할 부분!!!
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Equipment|Weapon")
	FName ThirdPersonWeaponAttachPoint = TEXT("hand_r");
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Equipment|Weapon")
	TMap<EBaruEquipmentSlot, FName> HandAttachPointBySlot;
	
	// 장비 해제 관련.
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UnequipWeapon(EBaruEquipmentSlot WeaponSlot);
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UnequipWeaponAtCell(EBaruEquipmentSlot WeaponSlot,FIntPoint TargetCell);

	// UFUNCTION이 아닌 서버 내부 공통 함수
	bool UnequipWeaponInternal(	EBaruEquipmentSlot WeaponSlot,	const FIntPoint* PreferredCell);
	
		// Character가 실제로 제거될 때, 장착 무기 Actor도 서버에서 정리.
		// 안 쓰면 캐릭터 사망 후에도 무기가 레벨에 남게됨.
	virtual void EndPlay(
		const EEndPlayReason::Type EndPlayReason) override;
	
	
	// 비활성 무기 장착 부분.
public:
	UFUNCTION(BlueprintPure, Category = "BARU|Equipment")
	EBaruEquipmentSlot GetActiveWeaponSlot() const
	{
		return ActiveWeaponSlot;
	}

		// 입력 또는 UI가 호출하는 무기 전환 요청 함수
	UFUNCTION(BlueprintCallable, Category = "BARU|Equipment|Weapon")
	void RequestSetActiveWeaponSlot(EBaruEquipmentSlot NewWeaponSlot);
	
	UFUNCTION(BlueprintCallable, Category = "BARU|Equipment|Weapon")
	void RequestUnequipWeaponAtCell(
		EBaruEquipmentSlot WeaponSlot,
		FIntPoint TargetCell);
	
	//장착 무기 떨어뜨리기(Grid에 반환하지 않고 정리하는 서버 함수)
	// Inventory의 서버 월드 드롭에서만 호출.
	// 보유 슬롯/아이템은 Inventory가 제거하고, 이 함수는 장착 외형과 참조만 정리.
	bool ReleaseWeaponForWorldDropOnServer(UBaruItemInstance* SourceItem);

protected:
	UFUNCTION(Server, Reliable)
	void Server_SetActiveWeaponSlot(EBaruEquipmentSlot NewWeaponSlot);

		// 서버에서 실제 무기 전환을 처리
	void SetActiveWeaponSlotOnServer(EBaruEquipmentSlot NewWeaponSlot);
	
	FName GetHandAttachPointForSlot(EBaruEquipmentSlot WeaponSlot) const;

		// 활성 무기: 손에 부착
	void AttachWeaponToHand(ABaruWeaponBase* Weapon);

		// 비활성 무기: DataAsset에 지정된 등/허리 소켓에 부착
	void AttachWeaponToHolster(ABaruWeaponBase* Weapon);

	TSubclassOf<UGameplayAbility> GetActiveWeaponFireAbilityClass() const;
	
	UFUNCTION()
	void OnRep_EquipmentState();
	
	
	// [09.13] 재장전 GA & ASC 연동 추가
protected:
	// 현재 부여된 Reload GA 핸들 및 클래스 보관
	FGameplayAbilitySpecHandle ActiveReloadAbilityHandle;
	UPROPERTY(Transient)
	TSubclassOf<UGameplayAbility> PrimaryReloadAbilityClass;
	UPROPERTY(Transient)
	TSubclassOf<UGameplayAbility> SecondaryReloadAbilityClass;

	// 현재 활성 무기의 Reload GA 클래스 반환
	TSubclassOf<UGameplayAbility> GetActiveWeaponReloadAbilityClass() const;

public:
	// 재장전 입력 요청 함수
	UFUNCTION(BlueprintCallable, Category = "BARU|Equipment")
	void RequestReloadActiveWeapon();
	
	// 심리스 이동 후 bEquipped 상태로 복사된 아이템의 3D 무기 액터를 재생성하여 장착 복구
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "BARU|Equipment")
	bool RestoreEquippedWeapon(UBaruItemInstance* SourceItem);
};