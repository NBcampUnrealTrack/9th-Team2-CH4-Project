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
            return;
        }
    }

    // 복제 지연으로 Pawn이나 컴포넌트가 아직 준비되지 않았다면 0.1초 뒤 재시도
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
        if (!CachedEquipmentComp.IsValid()) return;
    }

    ABaruWeaponBase* NewWeapon = CachedEquipmentComp->GetActiveWeapon();

    // 동일한 무기를 계속 쥐고 있다면 바인딩 갱신 생략
    if (NewWeapon == CurrentBoundWeapon.Get())
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
        // 새 무기 이벤트 바인딩
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
    if (!IsValid(Weapon) || !CachedEquipmentComp.IsValid())
    {
        if (Text_WeaponName) Text_WeaponName->SetVisibility(ESlateVisibility::Collapsed);
        if (Text_Ammo) Text_Ammo->SetVisibility(ESlateVisibility::Collapsed);
        if (Img_WeaponIcon) Img_WeaponIcon->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }
    
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

    // 무기가 없거나 장착 해제된 경우 숨김
    if (!CurrentBoundWeapon.IsValid())
    {
        Text_Ammo->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    const FText AmmoText = FText::Format(FText::FromString(TEXT("{0} / {1}")), FText::AsNumber(Current), FText::AsNumber(Max));
    Text_Ammo->SetText(AmmoText);

    // 0발일 경우 빨간색 경고 색상 적용
    Text_Ammo->SetColorAndOpacity(Current <= 0 ? EmptyAmmoColor : NormalAmmoColor); 
    Text_Ammo->SetVisibility(ESlateVisibility::HitTestInvisible);
}