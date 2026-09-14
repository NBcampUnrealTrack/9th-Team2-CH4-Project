// BaruCharacterEquipmentWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/BaruCommonUserWidget.h"
#include "Gameplay/Equipment/DataTypes/BaruEquipmentTypes.h"
#include "BaruCharacterEquipmentWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UBaruItemInstance;
class UBaruEquipmentComponent;
class UBaruInventoryComponent;
class UButton;
class UDragDropOperation;

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
		//우클릭, 더블클릭 장비해제
	virtual FReply NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent) override;

	virtual FReply NativeOnMouseButtonDoubleClick(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;
	
		//드래그 앤 드랍
	virtual FReply NativeOnPreviewMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent) override;

	virtual FReply NativeOnMouseButtonUp(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;

	virtual void NativeOnDragDetected(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent,
		UDragDropOperation*& OutOperation) override;

	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;

private:
	void BindEquipment();
	
	UFUNCTION()
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
	
	EBaruEquipmentSlot FindWeaponSlotUnderMouse(
	const FVector2D& ScreenPosition) const;

	bool RequestUnequipSlot(EBaruEquipmentSlot WeaponSlot);
	
		//[장비]드래그 앤 드랍
	EBaruEquipmentSlot PendingWeaponDragSlot =
	EBaruEquipmentSlot::None;
};