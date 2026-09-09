// BaruInventoryCellWidget.cpp
// 이곳의 좌표는 드래그앤드롭에서 "마우스가 어느 칸 위에 있는가" 판정에 사용.

#include "UI/InventoryUI/BaruInventoryCellWidget.h"

void UBaruInventoryCellWidget::InitializeCell(
	FIntPoint InCellCoordinate)
{
	CellCoordinate = InCellCoordinate;
}