//BaruInventoryScreenWidget.cpp

#include "UI/InventoryUI/BaruInventoryScreenWidget.h"

UBaruInventoryScreenWidget::UBaruInventoryScreenWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputConfig = EBaruWidgetInputMode::GameAndMenu;

	bSupportsActivationFocus = false;
	bIsBackHandler = false;
	SetIsFocusable(false);
}