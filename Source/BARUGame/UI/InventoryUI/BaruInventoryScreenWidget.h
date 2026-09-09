//BaruInventoryScreenWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/BaruActivatableWidget.h"
#include "BaruInventoryScreenWidget.generated.h"

UCLASS(Abstract)
class BARUGAME_API UBaruInventoryScreenWidget
	: public UBaruActivatableWidget
{
	GENERATED_BODY()

public:
	UBaruInventoryScreenWidget(
		const FObjectInitializer& ObjectInitializer);
};