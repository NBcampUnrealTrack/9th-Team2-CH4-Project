// BaruEquipmentComponent.cpp


#include "Gameplay/Equipment/BaruEquipmentComponent.h"

#include "Net/UnrealNetwork.h"

#include "Gameplay/Weapon/BaruWeaponBase.h"
#include "Gameplay/Weapon/Data/BaruWeaponDataAsset.h"

#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

#include "BaruLog.h"

	// 장비는 매 프레임 계산할 일이 없으므로 Tick을 사용 않함.
UBaruEquipmentComponent::UBaruEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

		// 이 컴포넌트의 장착 상태를 네트워크 복제 대상으로 설정
	SetIsReplicatedByDefault(true);
}


	// 위에서 Replicated로 선언한 변수들을 실제 복제 목록에 등록.
void UBaruEquipmentComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UBaruEquipmentComponent, PrimaryWeapon);
	DOREPLIFETIME(UBaruEquipmentComponent, SecondaryWeapon);
	DOREPLIFETIME(UBaruEquipmentComponent, ActiveWeaponSlot);
}

    // 서버가 무기 DataAsset을 기준으로 Weapon Actor를 생성.
    // Character의 3인칭 Mesh에 부착.
bool UBaruEquipmentComponent::EquipWeapon(
    UBaruWeaponDataAsset* WeaponData)
{
       // 장착 판정은 서버.
    if (!GetOwner() || !GetOwner()->HasAuthority() || !WeaponData)
    {
        return false;
    }

      // 현재는 주무기와 보조무기만 장착 대상으로 허용. 테스트용.
        // 나중에는 다른 것도.
    const EBaruEquipmentSlot TargetSlot = WeaponData->EquipmentSlot;

    if (TargetSlot != EBaruEquipmentSlot::PrimaryWeapon
        && TargetSlot != EBaruEquipmentSlot::SecondaryWeapon)
    {
        BARU_NET_LOG(
            GetOwner(),
            LogBaruItem,
            Warning,
            TEXT("EquipWeapon 실패: WeaponData의 EquipmentSlot이 무기 슬롯이 아닙니다."));

        return false;
    }

        // DataAsset에 지정된 BP_Weapon_Revolver / BP_Weapon_Rifle 클래스를 부름.
    TSubclassOf<ABaruWeaponBase> WeaponClass =
        WeaponData->WeaponActorClass.LoadSynchronous();

    if (!WeaponClass)
    {
        BARU_NET_LOG(
            GetOwner(),
            LogBaruItem,
            Warning,
            TEXT("EquipWeapon 실패: WeaponActorClass가 비어 있습니다."));

        return false;
    }

        // EquipmentComponent는 Character에 찰싹.
        // 생명주기를 Character와 함께해야하니까.
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());

    if (!OwnerCharacter || !OwnerCharacter->GetMesh())
    {
        BARU_NET_LOG(
            GetOwner(),
            LogBaruItem,
            Warning,
            TEXT("EquipWeapon 실패: Character 또는 Character Mesh가 없습니다."));

        return false;
    }

    USkeletalMeshComponent* CharacterMesh = OwnerCharacter->GetMesh();

        // 소켓 또는 본 이름이 실제 Character Mesh에 있는지 확인.
        // .h에서 weapon_r로 했던 부분이 실제 소켓이나 본으로 있는지 확인.
    const bool bHasAttachPoint =
        CharacterMesh->DoesSocketExist(ThirdPersonWeaponAttachPoint)
        || CharacterMesh->GetBoneIndex(ThirdPersonWeaponAttachPoint) != INDEX_NONE;

    if (!bHasAttachPoint)
    {
        BARU_NET_LOG(
            GetOwner(),
            LogBaruItem,
            Warning,
            TEXT("EquipWeapon 실패: 부착 위치 '%s'를 찾지 못했습니다."),
            *ThirdPersonWeaponAttachPoint.ToString());

        return false;
    }

        // 새 무기 Actor 생성 시 사용할 정보.
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = OwnerCharacter;
    SpawnParams.Instigator = OwnerCharacter;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ABaruWeaponBase* NewWeapon =
        GetWorld()->SpawnActor<ABaruWeaponBase>(
            WeaponClass,
            FTransform::Identity,
            SpawnParams);

    if (!NewWeapon)
    {
        return false;
    }

        // 그냥 weaponbase에서 다 선언했었다면 필요없었을 부분.
        // WeaponBase의 런타임 값에 DataAsset의 피해량·사거리·탄창 수 등 정적 설정을 새 Actor에 복사.
        // 무기별 수치의 기준을 DataAsset 한 곳으로 유지하기 위함.
    NewWeapon->InitializeFromData(WeaponData);

        // Character의 weapon_r 위치에 무기를 맞춰 부착.
    NewWeapon->AttachToComponent(
        CharacterMesh,
        FAttachmentTransformRules::SnapToTargetNotIncludingScale,
        ThirdPersonWeaponAttachPoint);

        // 기존에 사용하던 무기는 숨깁니다.
    if (ActiveWeaponSlot == EBaruEquipmentSlot::PrimaryWeapon
        && IsValid(PrimaryWeapon))
    {
        PrimaryWeapon->SetActorHiddenInGame(true);
    }
    else if (ActiveWeaponSlot == EBaruEquipmentSlot::SecondaryWeapon
        && IsValid(SecondaryWeapon))
    {
        SecondaryWeapon->SetActorHiddenInGame(true);
    }

        // 같은 슬롯에 이미 장착된 무기가 있다면 교체하기 전에 제거.
        // 반쯤 오류 방지용. 같은 슬롯에 무기 Actor가 두 개 남는 것을 방지하는 용도.
    if (TargetSlot == EBaruEquipmentSlot::PrimaryWeapon)
    {
        if (IsValid(PrimaryWeapon))
        {
            PrimaryWeapon->Destroy();
        }

        PrimaryWeapon = NewWeapon;
    }
    else
    {
        if (IsValid(SecondaryWeapon))
        {
            SecondaryWeapon->Destroy();
        }

        SecondaryWeapon = NewWeapon;
    }

        // 새로 장착한 무기는 현재 사용 무기로 설정.
    NewWeapon->SetActorHiddenInGame(false);
    ActiveWeaponSlot = TargetSlot;

        // 장착 상태 변경을 즉시 네트워크 갱신 대상으로 표시.
    GetOwner()->ForceNetUpdate();

    BARU_NET_LOG(
        GetOwner(),
        LogBaruItem,
        Log,
        TEXT("Weapon 장착 성공: %s"),
        *GetNameSafe(NewWeapon));

    return true;
}

    // UnequipWeapon() : 지정 슬롯의 무기 Actor를 제거하고 변수도 비우는 함수.
    // 무기 버리거나, 다른 무기 교체, 캐릭 사망, 장비 해제 등일 때 사용.
void UBaruEquipmentComponent::UnequipWeapon(
    EBaruEquipmentSlot WeaponSlot)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    if (WeaponSlot == EBaruEquipmentSlot::PrimaryWeapon)
    {
        if (IsValid(PrimaryWeapon))
        {
            PrimaryWeapon->Destroy();
        }

        PrimaryWeapon = nullptr;
    }
    else if (WeaponSlot == EBaruEquipmentSlot::SecondaryWeapon)
    {
        if (IsValid(SecondaryWeapon))
        {
            SecondaryWeapon->Destroy();
        }

        SecondaryWeapon = nullptr;
    }

    if (ActiveWeaponSlot == WeaponSlot)
    {
        ActiveWeaponSlot = EBaruEquipmentSlot::None;
    }

    GetOwner()->ForceNetUpdate();
}