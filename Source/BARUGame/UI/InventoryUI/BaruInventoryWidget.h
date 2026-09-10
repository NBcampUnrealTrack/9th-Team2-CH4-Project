//BaruInventoryWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/BaruCommonUserWidget.h"
#include "Gameplay/Items/DataTypes/BaruItemData.h"
#include "BaruInventoryWidget.generated.h"

class UBaruInventoryComponent;
class UBaruItemInstance;
class UTexture2D;
class UUniformGridPanel;
class USizeBox;
class UBaruInventoryCellWidget;
class UCanvasPanel;
class UBaruInventoryItemWidget;


// 인벤토리의 실제 슬롯 데이터를 UI가 읽기 쉬운 형태로 정리한 값.
// 서버 데이터를 복사해 표시할 뿐, UI에서 직접 수정하지 않습니다.
USTRUCT(BlueprintType)
struct BARUGAME_API FBaruInventoryUIEntry
{
	GENERATED_BODY()

	// 사용·장착·버리기 요청 시 원본 아이템을 식별하기 위한 참조.
	UPROPERTY(BlueprintReadOnly, Category = "BARU|Inventory UI")
	TObjectPtr<UBaruItemInstance> Item = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "BARU|Inventory UI")
	FName ItemID = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "BARU|Inventory UI")
	FText ItemName;

	UPROPERTY(BlueprintReadOnly, Category = "BARU|Inventory UI")
	TObjectPtr<UTexture2D> Thumbnail = nullptr;

	// 인벤토리의 좌상단 배치 좌표.
	UPROPERTY(BlueprintReadOnly, Category = "BARU|Inventory UI")
	FIntPoint TopLeft = FIntPoint::ZeroValue;

	// 해당 아이템이 차지하는 칸 크기.
	UPROPERTY(BlueprintReadOnly, Category = "BARU|Inventory UI")
	FIntPoint GridSize = FIntPoint(1, 1);

	UPROPERTY(BlueprintReadOnly, Category = "BARU|Inventory UI")
	int32 Quantity = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BARU|Inventory UI")
	EItemType ItemType = EItemType::None;
};

UCLASS(Abstract)
class BARUGAME_API UBaruInventoryWidget
	: public UBaruCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "BARU|Inventory UI")
	UBaruInventoryComponent* GetInventoryComponent() const
	{
		return InventoryComponent;
	}
	
	// 현재 인벤토리의 실제 열·행 크기를 반환합니다.
	// UI가 7×10을 별도로 하드코딩하지 않도록 사용합니다.
	UFUNCTION(BlueprintPure, Category = "BARU|Inventory UI")
	FIntPoint GetGridDimensions() const;

	// 현재 슬롯을 아이콘·수량·배치 정보와 함께 UI에 전달합니다.
	UFUNCTION(BlueprintPure, Category = "BARU|Inventory UI")
	TArray<FBaruInventoryUIEntry> GetInventoryViewEntries() const;

	UFUNCTION(BlueprintImplementableEvent,
		Category = "BARU|Inventory UI")
	void RefreshInventoryView();
	
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	// WBP_BaruInventory Designer의 같은 이름 위젯과 자동 연결됩니다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> InventorySizeBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> InventoryGrid;

	// 빈 칸 외형으로 사용할 WBP_BaruInventoryCell 클래스.
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Inventory UI")
	TSubclassOf<UBaruInventoryCellWidget> InventoryCellWidgetClass;

	// 한 칸의 화면상 크기. 단위는 Slate UI 픽셀입니다.
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Inventory UI",
		meta = (ClampMin = "16.0"))
	float GridCellSize = 64.0f;
	
	// 빈 그리드 위에 실제 아이템들을 배치하는 Canvas입니다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> InventoryItemCanvas;

	// 아이템 한 개를 표시할 Widget Blueprint 클래스입니다.
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Inventory UI")
	TSubclassOf<UBaruInventoryItemWidget> InventoryItemWidgetClass;
	
	virtual bool NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation) override;

private:
	void BindInventory();
	void HandleInventoryUpdated();
	void RebuildEmptyGrid();
	void RebuildItemWidgets();

	UPROPERTY()
	TObjectPtr<UBaruInventoryComponent> InventoryComponent;
};