// BaruEquipmentComponent.cpp


#include "Gameplay/Equipment/BaruEquipmentComponent.h"

#include "Net/UnrealNetwork.h"

#include "Gameplay/Weapon/BaruWeaponBase.h"
#include "Gameplay/Weapon/Data/BaruWeaponDataAsset.h"

#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

    //GA 연결
#include "GameFramework/Pawn.h"
#include "Player/BaruPlayerState.h"
#include "AbilitySystem/BaruAbilitySystemComponent.h"
#include "GameplayTags/BaruGameplayTags.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySpec.h"
#include "Gameplay/Items/BaruItemInstance.h"

#include "Gameplay/Inventory/BaruInventoryComponent.h"

#include "BaruLog.h"

	// 장비는 매 프레임 계산할 일이 없으므로 Tick을 사용 않함.
UBaruEquipmentComponent::UBaruEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

		// 이 컴포넌트의 장착 상태를 네트워크 복제 대상으로 설정
	SetIsReplicatedByDefault(true);
}

UBaruInventoryComponent*
UBaruEquipmentComponent::GetOwnerInventoryComponent() const
{
    const ACharacter* OwnerCharacter =
        Cast<ACharacter>(GetOwner());

    ABaruPlayerState* BaruPS = OwnerCharacter
        ? OwnerCharacter->GetPlayerState<ABaruPlayerState>()
        : nullptr;

    return IsValid(BaruPS)
        ? BaruPS->GetInventoryComponent()
        : nullptr;
}

// 플레이어 캐릭터 사망 시 액터도 사라지게 하는 내용.
    // Owner Character가 Destroy될 때 호출됨.
    // 단순 사망 상태가 아니라 Actor가 실제로 사라질 때만 실행됨.
    // 부활 후 리스폰 할 때에도 무기가 사라지도록.
void UBaruEquipmentComponent::EndPlay(
    const EEndPlayReason::Type EndPlayReason)
{
    AActor* OwnerActor = GetOwner();

    if (OwnerActor && OwnerActor->HasAuthority())
    {
        // PlayerState를 다시 찾지 않고 저장된 ASC로 GA부터 정리.
        ClearActiveWeaponFireAbilityOnServer();

        UnequipWeapon(EBaruEquipmentSlot::PrimaryWeapon);
        UnequipWeapon(EBaruEquipmentSlot::SecondaryWeapon);
    }

    Super::EndPlay(EndPlayReason);
}


	// 위에서 Replicated로 선언한 변수들을 실제 복제 목록에 등록.
void UBaruEquipmentComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UBaruEquipmentComponent, PrimaryWeapon);
    DOREPLIFETIME(UBaruEquipmentComponent, SecondaryWeapon);

    DOREPLIFETIME(
        UBaruEquipmentComponent,
        PrimaryWeaponItem);

    DOREPLIFETIME(
        UBaruEquipmentComponent,
        SecondaryWeaponItem);

    DOREPLIFETIME(
        UBaruEquipmentComponent,
        ActiveWeaponSlot);
}

    // 서버가 무기 DataAsset을 기준으로 Weapon Actor를 생성.
    // Character의 3인칭 Mesh에 부착.
bool UBaruEquipmentComponent::EquipWeapon(
    UBaruWeaponDataAsset* WeaponData,
    UBaruItemInstance* SourceItem)
{
       // 장착 판정은 서버.
        // 인벤토리 ItemInstance가 없는 장착 요청은 받지 않음.
    if (!GetOwner() || !GetOwner()->HasAuthority() || !WeaponData || !IsValid(SourceItem))
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

    UBaruItemInstance* EquippedItem =
    TargetSlot == EBaruEquipmentSlot::PrimaryWeapon
    ? PrimaryWeaponItem.Get()
    : SecondaryWeaponItem.Get();

    ABaruWeaponBase* EquippedWeapon =
        TargetSlot == EBaruEquipmentSlot::PrimaryWeapon
        ? PrimaryWeapon.Get()
        : SecondaryWeapon.Get();

    // 아이템과 실제 무기 Actor가 모두 존재해야 이미 장착된 것으로 인정합니다.
    if (IsValid(SourceItem)
        && EquippedItem == SourceItem
        && IsValid(EquippedWeapon))
    {
        if (ActiveWeaponSlot != TargetSlot)
        {
            SetActiveWeaponSlotOnServer(TargetSlot);
        }

        return true;
    }
    
    UBaruInventoryComponent* InventoryComponent =
    GetOwnerInventoryComponent();

    if (!IsValid(InventoryComponent)
        || InventoryComponent->IsItemEquipped(SourceItem))
    {
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
    
    // 새 무기 액터 생성까지 성공한 뒤 격자를 비움.
    if (!InventoryComponent->SetItemEquipped(
            SourceItem,
            true))
    {
        NewWeapon->Destroy();
        return false;
    }

    // 같은 슬롯에 기존 무기가 있다면 인벤토리로 반환.
    if (IsValid(EquippedItem)
        && !InventoryComponent->SetItemEquipped(
            EquippedItem,
            false))
    {
        // 기존 무기를 반환할 공간이 없으면 새 장착 취소.
        InventoryComponent->SetItemEquipped(
            SourceItem,
            false);

        NewWeapon->Destroy();
        return false;
    }

    /* 비활성 무기를 캐릭터 메시에 붙이기로 한 다음 주석처리한 부분.
     * 만약 비활성 무기를 안 할 거라면 다시 복구할 것.
        // Character의 weapon_r 위치에 무기를 맞춰 부착.
    NewWeapon->AttachToComponent(
        CharacterMesh,
        FAttachmentTransformRules::SnapToTargetNotIncludingScale,
        ThirdPersonWeaponAttachPoint);
*/
    /* 비활성 무기도 착용하는 코드를 넣은 뒤, 주석처리한 부분.
     * 
        // 같은 슬롯에 이미 장착된 무기가 있다면 교체하기 전에 제거.
        // 반쯤 오류 방지용. 같은 슬롯에 무기 Actor가 두 개 남는 것을 방지하는 용도.
    if (TargetSlot == EBaruEquipmentSlot::PrimaryWeapon)
    {   if (IsValid(PrimaryWeapon)) {   PrimaryWeapon->Destroy();   }
        PrimaryWeapon = NewWeapon;   }
    else
    {   if (IsValid(SecondaryWeapon)) {    SecondaryWeapon->Destroy();   }
        SecondaryWeapon = NewWeapon;    }
    */
    
    //-------비활성 무기 장착 부분.
        // 같은 슬롯에 기존 무기가 있는지 확인
    ABaruWeaponBase* PreviousWeaponInSlot =
        TargetSlot == EBaruEquipmentSlot::PrimaryWeapon
        ? PrimaryWeapon.Get()
        : SecondaryWeapon.Get();
    
        // 교체되는 슬롯이 현재 손에 든 슬롯이었는지 기억
    const bool bReplacingActiveSlot =
        ActiveWeaponSlot == TargetSlot;
    
    // 새 무기 생성에 성공했으므로 기존 같은 슬롯 무기를 제거
    if (IsValid(PreviousWeaponInSlot))
    {
        PreviousWeaponInSlot->Destroy();
    }
    
    // 새 무기 Actor와 해당 무기의 발사 GA를 같은 슬롯에 보관.
    if (TargetSlot == EBaruEquipmentSlot::PrimaryWeapon)
    {
        PrimaryWeapon = NewWeapon;
        PrimaryWeaponItem = SourceItem;
        PrimaryFireAbilityClass = WeaponData->FireAbilityClass;
        // [09.13] 주무기 장전 어빌리티 보관
        PrimaryReloadAbilityClass = WeaponData->ReloadAbilityClass;
    }
    else
    {
        SecondaryWeapon = NewWeapon;
        SecondaryWeaponItem = SourceItem;
        SecondaryFireAbilityClass = WeaponData->FireAbilityClass;
        // [09.13] 보조무기 장전 어빌리티 보관
        SecondaryReloadAbilityClass = WeaponData->ReloadAbilityClass;
    }
    
    NewWeapon->SetActorHiddenInGame(false);


        // 첫 무기 또는 현재 사용 중인 슬롯의 교체 무기는 손에 부착
    const bool bShouldAttachToHand =
        ActiveWeaponSlot == EBaruEquipmentSlot::None
        || bReplacingActiveSlot;

    if (bShouldAttachToHand)
    {
        ActiveWeaponSlot = TargetSlot;
        AttachWeaponToHand(NewWeapon);

         // 손에 사용하는 무기가 바뀌었으므로 발사 GA도 변경.
        SyncActiveWeaponFireAbilityOnServer();
    }
    else
    {
         // 홀스터에만 추가하는 무기는 현재 발사 GA를 바꾸지 않음.
        AttachWeaponToHolster(NewWeapon);
    }

    GetOwner()->ForceNetUpdate();
    
        // Listen Server의 로컬 UI는 서버에서 직접 갱신합니다.
        // 일반 클라이언트는 OnRep_EquipmentState에서 갱신됩니다.
    OnEquipmentUpdated.Broadcast();
    //-------여기까지 비활성 무기 장착 부분.

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
void UBaruEquipmentComponent::UnequipWeapon(EBaruEquipmentSlot WeaponSlot)
{
    // 좌표를 전달하지 않으면 기존처럼 첫 빈칸에 반환합니다.
    UnequipWeaponInternal(WeaponSlot, nullptr);
}

bool UBaruEquipmentComponent::UnequipWeaponInternal( EBaruEquipmentSlot WeaponSlot,  const FIntPoint* PreferredCell)
{
    if (!IsValid(GetOwner())
        || !GetOwner()->HasAuthority())
    {
        return false;
    }

    if (WeaponSlot != EBaruEquipmentSlot::PrimaryWeapon
        && WeaponSlot != EBaruEquipmentSlot::SecondaryWeapon)
    {
        return false;
    }

    UBaruItemInstance* EquippedItem =
        GetEquippedWeaponItem(WeaponSlot);

    if (IsValid(EquippedItem))
    {
        UBaruInventoryComponent* InventoryComponent =
            GetOwnerInventoryComponent();

        if (!IsValid(InventoryComponent))
        {
            return false;
        }

        bool bReturnedToInventory = false;

        if (PreferredCell)
        {
            bReturnedToInventory =
                InventoryComponent->ReturnEquippedItemToCell(
                    EquippedItem,
                    *PreferredCell);
        }
        else
        {
            bReturnedToInventory =
                InventoryComponent->SetItemEquipped(
                    EquippedItem,
                    false);
        }

        // 공간이 없거나 다른 아이템과 겹치면
        // 무기 액터와 장비 상태도 그대로 유지합니다.
        if (!bReturnedToInventory)
        {
            BARU_NET_LOG(
                GetOwner(),
                LogBaruItem,
                Warning,
                TEXT(
                    "무기 장착 해제 실패: "
                    "인벤토리 반환 위치를 사용할 수 없습니다. "
                    "Slot=%d"),
                static_cast<int32>(WeaponSlot));

            return false;
        }
    }

    if (WeaponSlot == EBaruEquipmentSlot::PrimaryWeapon)
    {
        if (IsValid(PrimaryWeapon))
        {
            PrimaryWeapon->Destroy();
        }

        PrimaryWeapon = nullptr;
        PrimaryWeaponItem = nullptr;
        PrimaryFireAbilityClass = nullptr;
    }
    else
    {
        if (IsValid(SecondaryWeapon))
        {
            SecondaryWeapon->Destroy();
        }

        SecondaryWeapon = nullptr;
        SecondaryWeaponItem = nullptr;
        SecondaryFireAbilityClass = nullptr;
    }

    if (ActiveWeaponSlot == WeaponSlot)
    {
        ActiveWeaponSlot = EBaruEquipmentSlot::None;
        SyncActiveWeaponFireAbilityOnServer();
    }

    GetOwner()->ForceNetUpdate();
    OnEquipmentUpdated.Broadcast();

    BARU_NET_LOG(
        GetOwner(),
        LogBaruItem,
        Log,
        TEXT(
            "장비 해제 처리 완료: "
            "Slot=%d ActiveSlot=%d"),
        static_cast<int32>(WeaponSlot),
        static_cast<int32>(ActiveWeaponSlot));

    return true;
}


    //GAS
ABaruWeaponBase* UBaruEquipmentComponent::GetActiveWeapon() const
{
    switch (ActiveWeaponSlot)
    {
    case EBaruEquipmentSlot::PrimaryWeapon:
        return PrimaryWeapon;

    case EBaruEquipmentSlot::SecondaryWeapon:
        return SecondaryWeapon;

    default:
        return nullptr;
    }
}
    //GAS

UBaruItemInstance*
UBaruEquipmentComponent::GetEquippedWeaponItem(
    EBaruEquipmentSlot WeaponSlot) const
{
    switch (WeaponSlot)
    {
    case EBaruEquipmentSlot::PrimaryWeapon:
        return PrimaryWeaponItem;

    case EBaruEquipmentSlot::SecondaryWeapon:
        return SecondaryWeaponItem;

    default:
        return nullptr;
    }
}

void UBaruEquipmentComponent::OnRep_EquipmentState()
{
    OnEquipmentUpdated.Broadcast();
}


/*GA 연결로 인하여 해당 부분 수정.
    //Character가 발사 입력을 받았을 때 호출. 진입점.
void UBaruEquipmentComponent::RequestFireActiveWeapon()
{
    if (!IsValid(GetOwner()))
        { return; }
    
        // 리슨 서버 호스트 = 서버 그 자체. -> RPC를 거치지 않고 처리.
    if (GetOwner() -> HasAuthority())
    {
            //발사!
        FireActiveWeaponOnServer();
        return;
    }
    
        // 일반(다른) 클라들은 서버에 발사를 요청.
    Server_RequestFireActiveWeapon();   // 서버_ 응답 -> 발싸!!! 액티브 웨폰.
    
}
*/
    //GA 사격 시작.
void UBaruEquipmentComponent::RequestFireActiveWeapon()
{
    APawn* Pawn = Cast<APawn>(GetOwner());
    if (!IsValid(Pawn) || !Pawn->IsLocallyControlled())
    {
        return;
    }

    ABaruPlayerState* PS = Pawn->GetPlayerState<ABaruPlayerState>();
    UBaruAbilitySystemComponent* ASC =
        PS ? PS->GetBaruAbilitySystemComponent() : nullptr;

    if (!IsValid(ASC))
    {
        BARU_NET_LOG(Pawn, LogBaruGAS, Warning,
            TEXT("[TEST] 발사 입력 실패: ASC 없음"));
        return;
    }

    BARU_NET_LOG(Pawn, LogBaruGAS, Log,
        TEXT("[TEST] 발사 입력 → ASC"));

    ASC->AbilityInputTagPressed(
        FBaruGameplayTags::Get().InputTag_Ability_Primary);
}
    // GA - 연사 멈춤.
void UBaruEquipmentComponent::RequestStopFireActiveWeapon()
{
    APawn* Pawn = Cast<APawn>(GetOwner());
    if (!IsValid(Pawn) || !Pawn->IsLocallyControlled())
    {
        return;
    }

    ABaruPlayerState* PS = Pawn->GetPlayerState<ABaruPlayerState>();
    UBaruAbilitySystemComponent* ASC =
        PS ? PS->GetBaruAbilitySystemComponent() : nullptr;

    if (IsValid(ASC))
    {
        ASC->AbilityInputTagReleased(
            FBaruGameplayTags::Get().InputTag_Ability_Primary);
    }
}

/*GA 발사로 전환하여 기존 WeaponBase 직접 발사 경로가 불필요해짐.
        //RPC 데이터에는 별도 입력값이 없음. -> 최소한 컴포넌트의 Owner가 정상인지 확인 필요.
bool UBaruEquipmentComponent::Server_RequestFireActiveWeapon_Validate()
{
    return IsValid(GetOwner());
}


        // 서버에서 실제 검증 함수 호출.
void UBaruEquipmentComponent::Server_RequestFireActiveWeapon_Implementation()
{
    FireActiveWeaponOnServer();
}

        // 서버가 현재 장착된 무기를 직접 찾아서 발사.
void UBaruEquipmentComponent::FireActiveWeaponOnServer()
{
    AActor* OwnerActor = GetOwner();

    if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority())
    {
        return;
    }

    ABaruWeaponBase* ActiveWeapon = GetActiveWeapon();

    if (!IsValid(ActiveWeapon))
    {
        return;
    }
    
        // 다른 클라(플레이어)들의 무기를 발사하는 비정상 요청은 방지.
    if (ActiveWeapon ->GetOwner() != OwnerActor)
    {
        BARU_NET_LOG(
            OwnerActor,
            LogBaruItem,
            Warning,
            TEXT("발사 거절 : 현재 Character가 소유한 무기가 아닙니다."));
        
        return;
    }
    
        //기존의 WeaponBase의 실제 발사 함수 호출 부분.
    ActiveWeapon -> Fire(OwnerActor);
}

*/


    // 비활성 무기칸.
void UBaruEquipmentComponent::RequestSetActiveWeaponSlot(
    EBaruEquipmentSlot NewWeaponSlot)
{
    if (!IsValid(GetOwner()))
    {
        return;
    }

    // 서버라면 즉시 처리, 클라이언트라면 서버에 요청
    if (GetOwner()->HasAuthority())
    {
        SetActiveWeaponSlotOnServer(NewWeaponSlot);
    }
    else
    {
        Server_SetActiveWeaponSlot(NewWeaponSlot);
    }
}

void UBaruEquipmentComponent::Server_SetActiveWeaponSlot_Implementation(
    EBaruEquipmentSlot NewWeaponSlot)
{
    SetActiveWeaponSlotOnServer(NewWeaponSlot);
}

void UBaruEquipmentComponent::SetActiveWeaponSlotOnServer(
    EBaruEquipmentSlot NewWeaponSlot)
{
    if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
    {
        return;
    }

        // 주무기·보조무기 이외의 슬롯 요청은 거절
    if (NewWeaponSlot != EBaruEquipmentSlot::PrimaryWeapon
        && NewWeaponSlot != EBaruEquipmentSlot::SecondaryWeapon)
    {
        return;
    }

    ABaruWeaponBase* NewActiveWeapon =
        (NewWeaponSlot == EBaruEquipmentSlot::PrimaryWeapon)
        ? PrimaryWeapon.Get()
        : SecondaryWeapon.Get();

        // 해당 슬롯에 실제 장착된 무기가 없으면 전환하지 않음
    if (!IsValid(NewActiveWeapon))
    {
        BARU_NET_LOG(
            GetOwner(), LogBaruItem, Warning,
            TEXT("무기 전환 실패: 해당 슬롯에 장착된 무기가 없습니다. Slot=%d"),
            static_cast<int32>(NewWeaponSlot));
        return;
    }

        // 이미 활성화된 무기라면 처리하지 않음
    if (ActiveWeaponSlot == NewWeaponSlot)
    {
        return;
    }

        // 기존 손 무기는 등/허리로 이동
    if (ABaruWeaponBase* PreviousWeapon = GetActiveWeapon())
    {
        AttachWeaponToHolster(PreviousWeapon);
    }

    // 새 무기는 손으로 이동.
    ActiveWeaponSlot = NewWeaponSlot;
    AttachWeaponToHand(NewActiveWeapon);

    // 손에 든 무기에 맞춰 발사 GA 변경.
    SyncActiveWeaponFireAbilityOnServer();

    GetOwner()->ForceNetUpdate();
    OnEquipmentUpdated.Broadcast();
}

    //무기를 손에 붙이기.
// ★[추가 09.14] 슬롯별 소켓이 지정돼 있으면 그걸, 아니면 기본 소켓(ThirdPersonWeaponAttachPoint)
FName UBaruEquipmentComponent::GetHandAttachPointForSlot(EBaruEquipmentSlot WeaponSlot) const
{
    if (const FName* SlotSocket = HandAttachPointBySlot.Find(WeaponSlot))
    {
        if (!SlotSocket->IsNone())
        {
            return *SlotSocket;
        }
    }
    return ThirdPersonWeaponAttachPoint;
}

//무기를 손에 붙이기.
void UBaruEquipmentComponent::AttachWeaponToHand(
    ABaruWeaponBase* Weapon)
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!IsValid(Weapon) || !IsValid(OwnerCharacter))
    {
        return;
    }

    USkeletalMeshComponent* CharacterMesh = OwnerCharacter->GetMesh();
    if (!IsValid(CharacterMesh))
    {
        return;
    }

    // [추가] 이 무기가 어느 슬롯인지 알아내서 슬롯별 손 소켓을 고릅니다.
    EBaruEquipmentSlot WeaponSlot = ActiveWeaponSlot;
    if (Weapon == PrimaryWeapon)
    {
        WeaponSlot = EBaruEquipmentSlot::PrimaryWeapon;
    }
    else if (Weapon == SecondaryWeapon)
    {
        WeaponSlot = EBaruEquipmentSlot::SecondaryWeapon;
    }
    const FName HandAttachPoint = GetHandAttachPointForSlot(WeaponSlot);

    // [수정] ThirdPersonWeaponAttachPoint → HandAttachPoint (아래 3군데)
    const bool bHasAttachPoint =
        CharacterMesh->DoesSocketExist(HandAttachPoint)
        || CharacterMesh->GetBoneIndex(HandAttachPoint) != INDEX_NONE;

    if (!bHasAttachPoint)
    {
        BARU_NET_LOG(
            GetOwner(), LogBaruItem, Warning,
            TEXT("손 부착 실패: 전신 메시에서 '%s'를 찾지 못했습니다."),
            *HandAttachPoint.ToString());
        return;
    }

    const bool bAttached = Weapon->AttachToComponent(
        CharacterMesh,
        FAttachmentTransformRules::SnapToTargetNotIncludingScale,
        HandAttachPoint);

    if (!bAttached)
    {
        return;
    }

    Weapon->SetActorRelativeTransform(
        Weapon->GetHandRelativeTransform());

    Weapon->ForceNetUpdate();
}

    // 홀스터에 무기 붙이기.
void UBaruEquipmentComponent::AttachWeaponToHolster(
    ABaruWeaponBase* Weapon)
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!IsValid(Weapon) || !IsValid(OwnerCharacter))
    {
        return;
    }

    
    
    const FName HolsterSocketName = Weapon->GetHolsterSocketName();    
    if (HolsterSocketName.IsNone())
    {
        BARU_NET_LOG(
            GetOwner(),
            LogBaruItem,
            Warning,
            TEXT("홀스터 부착 실패: %s의 HolsterSocketName이 None입니다."),
            *GetNameSafe(Weapon));

        return;
    }

    USkeletalMeshComponent* CharacterMesh = OwnerCharacter->GetMesh();
    if (!IsValid(CharacterMesh))
    {
        return;
    }

    const bool bHasAttachPoint =
        CharacterMesh->DoesSocketExist(HolsterSocketName)
        || CharacterMesh->GetBoneIndex(HolsterSocketName) != INDEX_NONE;

    if (!bHasAttachPoint)
    {
        BARU_NET_LOG(
            GetOwner(),
            LogBaruItem,
            Warning,
            TEXT("홀스터 부착 실패: Character Mesh에서 '%s'를 찾지 못했습니다."),
            *HolsterSocketName.ToString());

        return;
    }

    const bool bAttached = Weapon->AttachToComponent(
    CharacterMesh,
    FAttachmentTransformRules::SnapToTargetNotIncludingScale,
    HolsterSocketName);

    if (!bAttached)
    {
        BARU_NET_LOG(
            GetOwner(),
            LogBaruItem,
            Warning,
            TEXT("홀스터 부착 실패: AttachToComponent 실패. Weapon=%s, Socket=%s"),
            *GetNameSafe(Weapon),
            *HolsterSocketName.ToString());

        return;
    }

    Weapon->SetActorRelativeTransform(
        Weapon->GetHolsterRelativeTransform());

    BARU_NET_LOG(
        GetOwner(),
        LogBaruItem,
        Log,
        TEXT("홀스터 부착 성공: Weapon=%s, Socket=%s"),
        *GetNameSafe(Weapon),
        *HolsterSocketName.ToString());
}

    //GA 추가 위해. DA는 외형 담당.
TSubclassOf<UGameplayAbility>
UBaruEquipmentComponent::GetActiveWeaponFireAbilityClass() const
{
    switch (ActiveWeaponSlot)
    {
    case EBaruEquipmentSlot::PrimaryWeapon:
        return PrimaryFireAbilityClass;

    case EBaruEquipmentSlot::SecondaryWeapon:
        return SecondaryFireAbilityClass;

    default:
        return nullptr;
    }
}


void UBaruEquipmentComponent::ClearActiveWeaponFireAbilityOnServer()
{
    AActor* OwnerActor = GetOwner();

    if (!OwnerActor || !OwnerActor->HasAuthority())
    {
        return;
    }

    UBaruAbilitySystemComponent* GrantedASC =
        ActiveFireAbilityASC.Get();

    const FGameplayAbilitySpecHandle HandleToRemove =
        ActiveFireAbilityHandle;
    
    // [09.13] 재장전 핸들러 추가
    const FGameplayAbilitySpecHandle ReloadHandle = ActiveReloadAbilityHandle;

    // 취소 과정에서 다른 처리가 실행되더라도 같은 핸들을 다시 제거하지 않도록
    // 내부 기록부터 비웁니다.
    ActiveFireAbilityHandle = FGameplayAbilitySpecHandle();
    // [09.13] 재장전 핸들러 추가
    ActiveReloadAbilityHandle = FGameplayAbilitySpecHandle();
    ActiveFireAbilityASC.Reset();

    if (!IsValid(GrantedASC) || !HandleToRemove.IsValid())
    {
        return;
    }

    GrantedASC->CancelAbilityHandle(HandleToRemove);
    GrantedASC->ClearAbility(HandleToRemove);
    
    // [09.13] Reload 어빌리티 해제
    if (ReloadHandle.IsValid())
    {
        GrantedASC->CancelAbilityHandle(ReloadHandle);
        GrantedASC->ClearAbility(ReloadHandle);
    }
}

void UBaruEquipmentComponent::SyncActiveWeaponFireAbilityOnServer()
{
    AActor* OwnerActor = GetOwner();

    if (!OwnerActor || !OwnerActor->HasAuthority())
    {
        return;
    }

    ABaruWeaponBase* ActiveWeapon = GetActiveWeapon();

    // 무기 해제 시에는 현재 PlayerState 연결 여부와 무관하게
    // 저장해 둔 ASC에서 기존 발사 GA를 제거합니다.
    if (!IsValid(ActiveWeapon))
    {
        ClearActiveWeaponFireAbilityOnServer();
        return;
    }

    const TSubclassOf<UGameplayAbility> FireClass =
        GetActiveWeaponFireAbilityClass();
    
    // [09.13] 재장전 추가
    const TSubclassOf<UGameplayAbility> ReloadClass = GetActiveWeaponReloadAbilityClass(); // [수정]

    if (!FireClass || FireClass->HasAnyClassFlags(CLASS_Abstract))
    {
        ClearActiveWeaponFireAbilityOnServer();

        BARU_NET_LOG(
            OwnerActor, LogBaruGAS, Warning,
            TEXT("Fire GA 연결 실패: FireAbilityClass 미지정 또는 추상 클래스. Weapon=%s"),
            *GetNameSafe(ActiveWeapon));

        return;
    }

    ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerActor);

    ABaruPlayerState* PS = OwnerCharacter
        ? OwnerCharacter->GetPlayerState<ABaruPlayerState>()
        : nullptr;

    UBaruAbilitySystemComponent* ASC = IsValid(PS)
        ? PS->GetBaruAbilitySystemComponent()
        : nullptr;

    // [09.13] 복잡한 주석을 정리하고 재장전 로직 추가
    if (!IsValid(ASC))
    {
        ClearActiveWeaponFireAbilityOnServer();
        return;
    }

    // 기존 GA 해제 후 새로 부여
    ClearActiveWeaponFireAbilityOnServer();

    // 1. 발사 GA 부여 (InputTag_Ability_Primary)
    if (FireClass && !FireClass->HasAnyClassFlags(CLASS_Abstract))
    {
        FGameplayAbilitySpec FireSpec(FireClass, 1, INDEX_NONE, ActiveWeapon);
        FireSpec.GetDynamicSpecSourceTags().AddTag(FBaruGameplayTags::Get().InputTag_Ability_Primary);
        ActiveFireAbilityHandle = ASC->GiveAbility(FireSpec);
    }

    // 2. 재장전 GA 부여 (InputTag_Reload)
    if (ReloadClass && !ReloadClass->HasAnyClassFlags(CLASS_Abstract))
    {
        FGameplayAbilitySpec ReloadSpec(ReloadClass, 1, INDEX_NONE, ActiveWeapon);
        ReloadSpec.GetDynamicSpecSourceTags().AddTag(FBaruGameplayTags::Get().InputTag_Reload);
        ActiveReloadAbilityHandle = ASC->GiveAbility(ReloadSpec);
    }

    ActiveFireAbilityASC = ASC;
}
    // 장비 해제 관련.
void UBaruEquipmentComponent::RequestUnequipWeapon( EBaruEquipmentSlot WeaponSlot)
{
    if (!IsValid(GetOwner()))
    {
        return;
    }

    if (WeaponSlot != EBaruEquipmentSlot::PrimaryWeapon
        && WeaponSlot != EBaruEquipmentSlot::SecondaryWeapon)
    {
        return;
    }

    if (GetOwner()->HasAuthority())
    {
        UnequipWeapon(WeaponSlot);
        return;
    }

    const APawn* OwnerPawn = Cast<APawn>(GetOwner());

    if (IsValid(OwnerPawn) && OwnerPawn->IsLocallyControlled())
    {
        Server_UnequipWeapon(WeaponSlot);
    }
}

void UBaruEquipmentComponent::RequestUnequipWeaponAtCell( EBaruEquipmentSlot WeaponSlot, FIntPoint TargetCell)
{
    if (!IsValid(GetOwner()))
    {
        return;
    }

    if (WeaponSlot != EBaruEquipmentSlot::PrimaryWeapon
        && WeaponSlot != EBaruEquipmentSlot::SecondaryWeapon)
    {
        return;
    }

    if (GetOwner()->HasAuthority())
    {
        UnequipWeaponInternal(
            WeaponSlot,
            &TargetCell);

        return;
    }

    const APawn* OwnerPawn =
        Cast<APawn>(GetOwner());

    if (IsValid(OwnerPawn)
        && OwnerPawn->IsLocallyControlled())
    {
        Server_UnequipWeaponAtCell(
            WeaponSlot,
            TargetCell);
    }
}

bool UBaruEquipmentComponent::
Server_UnequipWeaponAtCell_Validate(
    EBaruEquipmentSlot WeaponSlot,
    FIntPoint TargetCell)
{
    const bool bValidSlot =
        WeaponSlot == EBaruEquipmentSlot::PrimaryWeapon
        || WeaponSlot == EBaruEquipmentSlot::SecondaryWeapon;

    const UBaruInventoryComponent* InventoryComponent =
        GetOwnerInventoryComponent();

    return bValidSlot
        && IsValid(InventoryComponent)
        && TargetCell.X >= 0
        && TargetCell.Y >= 0
        && TargetCell.X < InventoryComponent->GridWidth
        && TargetCell.Y < InventoryComponent->GridHeight;
}

void UBaruEquipmentComponent::
Server_UnequipWeaponAtCell_Implementation(
    EBaruEquipmentSlot WeaponSlot,
    FIntPoint TargetCell)
{
    UnequipWeaponInternal(
        WeaponSlot,
        &TargetCell);
}

bool UBaruEquipmentComponent::Server_UnequipWeapon_Validate(
    EBaruEquipmentSlot WeaponSlot)
{
    return WeaponSlot == EBaruEquipmentSlot::PrimaryWeapon
        || WeaponSlot == EBaruEquipmentSlot::SecondaryWeapon;
}

void UBaruEquipmentComponent::Server_UnequipWeapon_Implementation(
    EBaruEquipmentSlot WeaponSlot)
{
    UnequipWeapon(WeaponSlot);
}

    //드롭 추가
bool UBaruEquipmentComponent::ReleaseWeaponForWorldDropOnServer(
    UBaruItemInstance* SourceItem)
{
    if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority()
        || !IsValid(SourceItem))
    {
        return false;
    }

    EBaruEquipmentSlot RemovedSlot = EBaruEquipmentSlot::None;
    ABaruWeaponBase* RemovedWeapon = nullptr;

    if (PrimaryWeaponItem.Get() == SourceItem)
    {
        RemovedSlot = EBaruEquipmentSlot::PrimaryWeapon;
        RemovedWeapon = PrimaryWeapon.Get();
        PrimaryWeapon = nullptr;
        PrimaryWeaponItem = nullptr;
        PrimaryFireAbilityClass = nullptr;
    }
    else if (SecondaryWeaponItem.Get() == SourceItem)
    {
        RemovedSlot = EBaruEquipmentSlot::SecondaryWeapon;
        RemovedWeapon = SecondaryWeapon.Get();
        SecondaryWeapon = nullptr;
        SecondaryWeaponItem = nullptr;
        SecondaryFireAbilityClass = nullptr;
    }
    else
    {
        return false;
    }

    if (ActiveWeaponSlot == RemovedSlot)
    {
        ActiveWeaponSlot = EBaruEquipmentSlot::None;
        SyncActiveWeaponFireAbilityOnServer();
    }

    if (IsValid(RemovedWeapon))
    {
        RemovedWeapon->Destroy();
    }

    GetOwner()->ForceNetUpdate();
    // OnEquipmentUpdated는 Inventory 삭제 완료 후 호출합니다.
    return true;
}

// [09.13] 재장전 로직 추가

TSubclassOf<UGameplayAbility> UBaruEquipmentComponent::GetActiveWeaponReloadAbilityClass() const
{
    switch (ActiveWeaponSlot)
    {
    case EBaruEquipmentSlot::PrimaryWeapon:
        return PrimaryReloadAbilityClass;
    case EBaruEquipmentSlot::SecondaryWeapon:
        return SecondaryReloadAbilityClass;
    default:
        return nullptr;
    }
}

void UBaruEquipmentComponent::RequestReloadActiveWeapon()
{
    APawn* Pawn = Cast<APawn>(GetOwner());
    if (!IsValid(Pawn) || !Pawn->IsLocallyControlled()) return;

    ABaruPlayerState* PS = Pawn->GetPlayerState<ABaruPlayerState>();
    UBaruAbilitySystemComponent* ASC = PS ? PS->GetBaruAbilitySystemComponent() : nullptr;

    if (IsValid(ASC))
    {
        ASC->AbilityInputTagPressed(FBaruGameplayTags::Get().InputTag_Reload);
    }
}