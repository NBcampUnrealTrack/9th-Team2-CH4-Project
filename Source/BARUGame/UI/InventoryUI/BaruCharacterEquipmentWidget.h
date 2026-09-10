// BaruCharacterEquipmentWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/BaruCommonUserWidget.h"
#include "BaruCharacterEquipmentWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UBaruItemInstance;
class UBaruEquipmentComponent;
class UBaruInventoryComponent;
class UButton;

UCLASS(Abstract)
class BARUGAME_API UBaruCharacterEquipmentWidget
	: public UBaruCommonUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Border_PrimaryWeaponSlot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_PrimaryWeapon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_PrimaryWeaponName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Border_SecondaryWeaponSlot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_SecondaryWeapon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SecondaryWeaponName;

	UPROPERTY(
		EditDefaultsOnly,
		Category = "BARU|Equipment UI")
	FLinearColor ActiveSlotColor =
		FLinearColor(0.15f, 0.45f, 0.20f, 1.0f);

	UPROPERTY(
		EditDefaultsOnly,
		Category = "BARU|Equipment UI")
	FLinearColor InactiveSlotColor =
		FLinearColor(0.08f, 0.08f, 0.08f, 0.90f);
	
	UFUNCTION()
	void HandlePrimaryWeaponClicked();

	UFUNCTION()
	void HandleSecondaryWeaponClicked();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_PrimaryWeapon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_SecondaryWeapon;

private:
	void BindEquipment();
	void HandleEquipmentUpdated();
	void RefreshWeaponSlots();

	void SetWeaponSlotDisplay(
		UImage* TargetImage,
		UTextBlock* TargetNameText,
		UBaruItemInstance* EquippedItem);

	UPROPERTY()
	TObjectPtr<UBaruEquipmentComponent> EquipmentComponent;

	UPROPERTY()
	TObjectPtr<UBaruInventoryComponent> InventoryComponent;
};