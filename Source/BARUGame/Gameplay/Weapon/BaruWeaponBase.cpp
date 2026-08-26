// Fill out your copyright notice in the Description page of Project Settings.


#include "Gameplay/Weapon/BaruWeaponBase.h"

// Sets default values
ABaruWeaponBase::ABaruWeaponBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

void ABaruWeaponBase::Fire(AActor* WeaponInstigator)
{
	// TODO: 실제 발사 로직 (라인트레이스, 데미지 적용 등)은 추후 구현.
	// 지금은 컴파일 통과를 위한 뼈대만.
}