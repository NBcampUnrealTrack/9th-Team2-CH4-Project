// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/Weapon/BaruWeaponBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Gameplay/Weapon/Data/BaruWeaponDataAsset.h"
#include "Net/UnrealNetwork.h"

ABaruWeaponBase::ABaruWeaponBase()
{
		// 무기는 매 프레임 Tick할 필요 없음
	PrimaryActorTick.bCanEverTick = false;

		// 다른 플레이어에게도 장착 무기가 보이도록 Actor 복제
	bReplicates = true;

	// 무기는 Character에 붙어서 움직이므로 별도 위치 복제는 하지 않음
	SetReplicateMovement(false);

	// 무기의 보이는 몸체이자 RootComponent
	WeaponMesh =
		CreateDefaultSubobject<USkeletalMeshComponent>(
			TEXT("WeaponMesh"));

	SetRootComponent(WeaponMesh);

	// 장착 무기는 충돌 판정을 하지 않음
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);
}

void ABaruWeaponBase::Fire(AActor* WeaponInstigator)
{
	// TODO: 실제 발사 로직 (라인트레이스, 데미지 적용 등)은 추후 구현.
	// 지금은 컴파일 통과를 위한 뼈대만.
}

	// 서버가 DataAsset의 정적 설정값을,
	// 실제로 생성된 무기 Actor의 런타임 값으로 복사합니다.
void ABaruWeaponBase::InitializeFromData(
	const UBaruWeaponDataAsset* WeaponData)
{
		// 무기 수치는 서버가 결정.
	if (!HasAuthority() || !WeaponData)
	{
		return;
	}

	Damage = WeaponData->BaseDamage;
	Range = WeaponData->MaxRange;
	MagazineCapacity = WeaponData->MagazineCapacity;
	FireInterval = WeaponData->FireInterval;

		// 변경된 복제 값을 다음 네트워크 갱신 때 전달하도록 요청
	ForceNetUpdate();
}

	// 생명주기.
void ABaruWeaponBase::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaruWeaponBase, Damage);
	DOREPLIFETIME(ABaruWeaponBase, Range);
	DOREPLIFETIME(ABaruWeaponBase, MagazineCapacity);
	DOREPLIFETIME(ABaruWeaponBase, FireInterval);
}