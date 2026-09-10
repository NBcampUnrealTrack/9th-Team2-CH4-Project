// BaruWeaponBase.cpp


#include "Gameplay/Weapon/BaruWeaponBase.h"

#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Gameplay/Weapon/Data/BaruWeaponDataAsset.h"
#include "Net/UnrealNetwork.h"


ABaruWeaponBase::ABaruWeaponBase()
{
		// 무기는 매 프레임 Tick할 필요 없음
	PrimaryActorTick.bCanEverTick = false;

		// 다른 플레이어에게도 장착 무기가 보이도록 Actor 복제
	bReplicates = true;

// 서버의 3인칭 무기 부착·이동 상태를 클라이언트에도 반영.
	SetReplicateMovement(true);

	// 무기의 보이는 몸체이자 RootComponent
	WeaponMesh =
		CreateDefaultSubobject<USkeletalMeshComponent>(
			TEXT("WeaponMesh"));

	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionEnabled(
		ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);
}

	// 무기를 다른 사람한테도 보이도록.
void ABaruWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	TInlineComponentArray<UPrimitiveComponent*> Components;
	GetComponents(Components);

	for (UPrimitiveComponent* Component : Components)
	{
		if (!IsValid(Component))
		{
			continue;
		}

		Component->SetOnlyOwnerSee(false);
		
		// 일반 WeaponMesh는 bHideFromOwner 설정을 따름.
		Component->SetOwnerNoSee(bHideFromOwner);
		
		// 장착 외형은 물리 시뮬레이션과 명중 충돌을 담당하지 않음.
		Component->SetSimulatePhysics(false);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetGenerateOverlapEvents(false);
	}
}


	// 서버가 DataAsset의 정적 설정값을,
	// 실제로 생성된 무기 Actor의 런타임 값으로 복사합니다.
void ABaruWeaponBase::InitializeFromData(
	const UBaruWeaponDataAsset* WeaponData)
{
		// 무기 수치는 서버가 결정.
	if (!HasAuthority() || !IsValid(WeaponData))
	{
		return;
	}

	//----- 비활성 무기 부착
		// 무기별 비활성 보관 위치 정보도 런타임 Weapon Actor에 복사.
		// 장착 보정값은 서버 EquipmentComponent가 사용.

	HolsterSocketName = WeaponData->HolsterSocketName;
	HolsterRelativeTransform = WeaponData->HolsterRelativeTransform;
	HandRelativeTransform = WeaponData->HandRelativeTransform;
	
	// 기존 탄창 설정 보존. 현재 탄약 소모 기능과는 별개.
	MagazineCapacity = WeaponData->MagazineCapacity;

	ForceNetUpdate();
}

	// 생명주기.
void ABaruWeaponBase::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaruWeaponBase, MagazineCapacity);
}


USceneComponent*
ABaruWeaponBase::GetFireEffectAttachComponent() const
{
	static const FName MuzzleSocket(TEXT("Muzzle"));

	// 우선 기본 WeaponMesh의 총구를 사용.
	if (IsValid(WeaponMesh)
		&& WeaponMesh->DoesSocketExist(MuzzleSocket))
	{
		return WeaponMesh.Get();
	}

	// BP에서 일반 StaticMesh를 외형으로 사용하는 경우.
	TInlineComponentArray<UMeshComponent*> MeshComponents;
	GetComponents(MeshComponents);

	for (UMeshComponent* Mesh : MeshComponents)
	{
		if (!IsValid(Mesh) || Mesh == WeaponMesh.Get())
		{
			continue;
		}

		if (Mesh->DoesSocketExist(MuzzleSocket))
		{
			return Mesh;
		}
	}

	return nullptr;
}