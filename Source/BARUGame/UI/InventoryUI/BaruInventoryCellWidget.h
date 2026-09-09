// BaruInventoryCellWidget.h
	// 인벤의 칸 하나.
	// 창을 여닫는 UI가 아니라, 인벤 안의 칸 하나이기 때문에 CommonUserWidget을 상속.
	// 

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/BaruCommonUserWidget.h"
#include "BaruInventoryCellWidget.generated.h"

UCLASS(Abstract)
class BARUGAME_API UBaruInventoryCellWidget
	: public UBaruCommonUserWidget
{
	GENERATED_BODY()

public:
	// 이 칸이 인벤토리의 몇 번째 좌표인지 저장합니다.
	void InitializeCell(FIntPoint InCellCoordinate);

	UFUNCTION(BlueprintPure, Category = "BARU|Inventory UI")
	FIntPoint GetCellCoordinate() const
	{
		return CellCoordinate;
	}

private:
	UPROPERTY(VisibleInstanceOnly, Category = "BARU|Inventory UI")
	FIntPoint CellCoordinate = FIntPoint::ZeroValue;
};