#include "UI/HUD/BaruWeaponAmmoWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Character/BaruCharacter.h"
#include "Player/BaruPlayerState.h"
#include "Gameplay/Equipment/BaruEquipmentComponent.h"
#include "Gameplay/Inventory/BaruInventoryComponent.h"
#include "Gameplay/Items/BaruItemInstance.h"
#include "Gameplay/Items/DataTypes/BaruItemData.h"
#include "Gameplay/Weapon/BaruWeaponBase.h"

void UBaruWeaponAmmoWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 생성 즉시 기본 상태를 숨김(Collapsed)으로 초기화하여 더미 위젯 노출 방지
    SetVisibility(ESlateVisibility::Collapsed);
    if (Text_WeaponName) Text_WeaponName->SetVisibility(ESlateVisibility::Collapsed);
    if (Text_Ammo) Text_Ammo->SetVisibility(ESlateVisibility::Collapsed);
    if (Img_WeaponIcon) Img_WeaponIcon->SetVisibility(ESlateVisibility::Collapsed);

    InitEquipmentBinding();
    BindToActiveWeapon();
}

void UBaruWeaponAmmoWidget::NativeDestruct()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(RetryInitTimerHandle);
    }

    if (CachedEquipmentComp.IsValid())
    {
        CachedEquipmentComp->OnEquipmentUpdated.RemoveDynamic(this, &UBaruWeaponAmmoWidget::HandleEquipmentUpdated);
    }

    if (CurrentBoundWeapon.IsValid())
    {
        CurrentBoundWeapon->OnAmmoChanged.RemoveDynamic(this, &UBaruWeaponAmmoWidget::HandleAmmoChanged);
    }

    CachedEquipmentComp.Reset();
    CurrentBoundWeapon.Reset();

    Super::NativeDestruct();
}

void UBaruWeaponAmmoWidget::InitEquipmentBinding()
{
    APawn* OwningPawn = GetOwningPlayerPawn();
    if (ABaruCharacter* Character = Cast<ABaruCharacter>(OwningPawn))
    {
        CachedEquipmentComp = Character->FindComponentByClass<UBaruEquipmentComponent>();
        if (CachedEquipmentComp.IsValid())
        {
            CachedEquipmentComp->OnEquipmentUpdated.AddUniqueDynamic(this, &UBaruWeaponAmmoWidget::HandleEquipmentUpdated);
            
            // 컴포넌트를 찾은 즉시 무기 상태를 반영하도록 호출
            BindToActiveWeapon();
            return;
        }
    }

    // 복제 지연으로 아직 준비되지 않았다면 0.1초 뒤 재시도
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(RetryInitTimerHandle, this, &UBaruWeaponAmmoWidget::InitEquipmentBinding, 0.1f, false);
    }
}

void UBaruWeaponAmmoWidget::HandleEquipmentUpdated()
{
    BindToActiveWeapon();
}

void UBaruWeaponAmmoWidget::BindToActiveWeapon()
{
    if (!CachedEquipmentComp.IsValid())
    {
        InitEquipmentBinding();
        if (!CachedEquipmentComp.IsValid())
        {
            UpdateWeaponDisplay(nullptr);
            return;
        }
    }

    ABaruWeaponBase* NewWeapon = CachedEquipmentComp->GetActiveWeapon();

    // 무기를 들고 있고 기존과 동일한 무기인 경우에만 스킵
    // (맨손인 NewWeapon == nullptr 상태에서는 아래 초기화 로직을 반드시 통과해야 함)
    if (NewWeapon != nullptr && NewWeapon == CurrentBoundWeapon.Get())
    {
        return;
    }

    // 기존 무기 이벤트 언바인딩
    if (CurrentBoundWeapon.IsValid())
    {
        CurrentBoundWeapon->OnAmmoChanged.RemoveDynamic(this, &UBaruWeaponAmmoWidget::HandleAmmoChanged);
    }

    CurrentBoundWeapon = NewWeapon;
    UpdateWeaponDisplay(NewWeapon);

    if (IsValid(NewWeapon))
    {
        NewWeapon->OnAmmoChanged.AddUniqueDynamic(this, &UBaruWeaponAmmoWidget::HandleAmmoChanged);
        UpdateAmmoDisplay(NewWeapon->GetCurrentAmmo(), NewWeapon->GetMagazineCapacity());
    }
}

void UBaruWeaponAmmoWidget::HandleAmmoChanged(int32 CurrentAmmo, int32 MaxCapacity)
{
    UpdateAmmoDisplay(CurrentAmmo, MaxCapacity);
}

void UBaruWeaponAmmoWidget::UpdateWeaponDisplay(ABaruWeaponBase* Weapon)
{
    // 맨손 상태이거나 유효하지 않은 무기인 경우 -> 배경 박스 및 모든 자식 위젯을 숨김
    if (!IsValid(Weapon) || !CachedEquipmentComp.IsValid())
    {
        SetVisibility(ESlateVisibility::Collapsed);
        if (Text_WeaponName) Text_WeaponName->SetVisibility(ESlateVisibility::Collapsed);
        if (Text_Ammo) Text_Ammo->SetVisibility(ESlateVisibility::Collapsed);
        if (Img_WeaponIcon) Img_WeaponIcon->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    // 무기가 존재하므로 위젯 전체를 표시
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    // 활성 슬롯에 장착된 아이템 인스턴스 조회
    const EBaruEquipmentSlot ActiveSlot = CachedEquipmentComp->GetActiveWeaponSlot(); 
    UBaruItemInstance* ItemInstance = CachedEquipmentComp->GetEquippedWeaponItem(ActiveSlot);

    ABaruPlayerState* PS = GetOwningPlayerState<ABaruPlayerState>();
    UBaruInventoryComponent* InvenComp = PS ? PS->GetInventoryComponent() : nullptr;

    if (ItemInstance && InvenComp && InvenComp->ItemDataTable)
    {
        const FItemData* ItemData = InvenComp->ItemDataTable->FindRow<FItemData>(ItemInstance->ItemID, TEXT("AmmoWidget"), false);
        if (ItemData)
        {
            if (Text_WeaponName)
            {
                Text_WeaponName->SetText(ItemData->ItemName);
                Text_WeaponName->SetVisibility(ESlateVisibility::HitTestInvisible);
            }

            if (Img_WeaponIcon)
            {
                if (ItemData->Thumbnail)
                {
                    Img_WeaponIcon->SetBrushFromTexture(ItemData->Thumbnail);
                    Img_WeaponIcon->SetVisibility(ESlateVisibility::HitTestInvisible); 
                }
                else 
                {
                    Img_WeaponIcon->SetVisibility(ESlateVisibility::Collapsed);
                }
            }
            return;
        }
    }

    // DataTable 조회 실패 시 기본 Fallback
    if (Text_WeaponName) 
    {
        Text_WeaponName->SetText(FText::FromString(TEXT("무기")));
        Text_WeaponName->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    if (Img_WeaponIcon) Img_WeaponIcon->SetVisibility(ESlateVisibility::Collapsed);
}

void UBaruWeaponAmmoWidget::UpdateAmmoDisplay(int32 Current, int32 Max)
{
    if (!Text_Ammo) return;

    if (!CurrentBoundWeapon.IsValid())
    {
        Text_Ammo->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    const FText AmmoText = FText::Format(FText::FromString(TEXT("{0} / {1}")), FText::AsNumber(Current), FText::AsNumber(Max));
    Text_Ammo->SetText(AmmoText);
    Text_Ammo->SetColorAndOpacity(Current <= 0 ? EmptyAmmoColor : NormalAmmoColor); 
    Text_Ammo->SetVisibility(ESlateVisibility::HitTestInvisible);
}