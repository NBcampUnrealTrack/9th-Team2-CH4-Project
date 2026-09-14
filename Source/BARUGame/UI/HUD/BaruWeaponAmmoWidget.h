#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/BaruCommonUserWidget.h"
#include "Styling/SlateColor.h"
#include "BaruWeaponAmmoWidget.generated.h"

class UTextBlock;
class UImage;
class ABaruWeaponBase;
class UBaruEquipmentComponent;

/**
 * 활성 무기의 탄약, 총기명, 썸네일 표시를 전담하는 C++ 위젯 베이스
 */
UCLASS(Abstract, Blueprintable)
class BARUGAME_API UBaruWeaponAmmoWidget : public UBaruCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// =========================================================================
	// UMG 디자이너 바인딩 위젯 (이름이 일치해야 함)
	// =========================================================================
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Ammo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_WeaponName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_WeaponIcon;

	// 0발 소진 시 탄약 숫자 텍스트 색상
	UPROPERTY(EditDefaultsOnly, Category = "BARU|UI|Ammo")
	FSlateColor EmptyAmmoColor = FSlateColor(FLinearColor(1.0f, 0.2f, 0.2f, 1.0f));

	UPROPERTY(EditDefaultsOnly, Category = "BARU|UI|Ammo")
	FSlateColor NormalAmmoColor = FSlateColor(FLinearColor::White);

protected:
	// 무기 변경 감지 이벤트 핸들러
	UFUNCTION()
	void HandleEquipmentUpdated();

	// 탄약 변경 감지 이벤트 핸들러
	UFUNCTION()
	void HandleAmmoChanged(int32 CurrentAmmo, int32 MaxCapacity);

private:
	void InitEquipmentBinding();
	void BindToActiveWeapon();
	void UpdateWeaponDisplay(ABaruWeaponBase* Weapon);
	void UpdateAmmoDisplay(int32 Current, int32 Max);

	UPROPERTY(Transient)
	TWeakObjectPtr<UBaruEquipmentComponent> CachedEquipmentComp;

	UPROPERTY(Transient)
	TWeakObjectPtr<ABaruWeaponBase> CurrentBoundWeapon;

	FTimerHandle RetryInitTimerHandle;
};