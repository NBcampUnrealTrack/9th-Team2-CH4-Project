// BaruInventoryComponent.h
// 인벤토리 코드의 메인.
// 강의와 다르게 그리드 인벤으로.
// 참고자료 : cpp - https://github.com/imnazake/grid-inventory-sample
// 블루프린트 - https://murlocdev.tistory.com/40

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Gameplay/Items/DataTypes/BaruItemData.h"   // FInventorySlot, FInventorySlotArray 사용 목적. 실제 경로에 맞게 수정.
#include "BaruInventoryComponent.generated.h"

class UDataTable;
class UBaruItemInstance;

DECLARE_MULTICAST_DELEGATE(FOnInventoryUpdated); // UI(태현님)와 맞춰서 이름 정할 것.

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent)) //ClassGroup은 그룹 정리용, meta = 부분은 에디터에서 BP에 붙이기 위함.
class BARUGAME_API UBaruInventoryComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UBaruInventoryComponent();
	
	//1. 멀티.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void InitializeComponent() override;
	virtual void ReadyForReplication() override;
	
	//2. 인벤토리 설정. 5x5였나.
	UPROPERTY(EditAnywhere, Category = "Grid", meta = (ClampMin = "1"))
	int32 GridWidth = 5;

	UPROPERTY(EditAnywhere, Category = "Grid", meta = (ClampMin = "1"))
	int32 GridHeight = 5;
	
		// DT_Items 할당
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	UDataTable* ItemDataTable = nullptr;
	
	//[09.03 추가. 무기 습득 및 장착.] [임시 테스트]
		// 무기 아이템을 습득했을 때 자동으로 장착할지 여부.
		// UI의 장착 버튼을 만들기 전, 전체 연결을 검증하기 위한 옵션.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|Inventory|Test")
	bool bAutoEquipWeaponOnPickupForTest = false;
	
	
	// 3. 슬롯 판정과 인벤 내 2D 겹침 검사. - 서버, 클라(클라는 UI 프리뷰용) 공용.
		// (1) 지정 좌표에 해당 크기의 아이템을 놓을 수 있는지 확인.
		// IsRoomAvailable : "이 방(자리)를 이용할 수 있나?(넣을 수 있나)"
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsRoomAvailable(FIntPoint TopLeft, FIntPoint Size,		// FIntPoint는 X, Y Int32
		const UBaruItemInstance* Ignore = nullptr) const; // 아이템을 옮길 때, 데이터상 남아있는 이동 이전의 자신과 겹침 판정을 막기 위해.
	
	
		// (2) 실제 자리 스캔 부분
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool FindFirstFit(FIntPoint Size, FIntPoint& OutTopLeft) const; // 처음으로 비어있는 자리 탐색.

	UFUNCTION(BlueprintPure, Category = "Inventory")
	UBaruItemInstance* GetItemAt(FIntPoint Cell) const;	// Get Item At : 특정 순서(인덱스)나 위치에 있는 항목을 가져올 때 사용. 여기선 특정 슬롯이 차있는지 확인용.

	//UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FInventorySlot>& GetSlots() const { return SlotList.Slots; }	// const로 가볍게, 슬롯 목적 전체를 UI쪽에서 읽을 수 있게 여는 함수. 수정은 불가.
	
		// (3) 변경 - 서버전용. UFUNCTION 없음 -> 서버 C++ 코드에서만 호출하는 내부함수.(HasAuthority 체크 필수)
		// UFUNCTION 등을 붙이면 클라 블프에서도 호출이 가능해지게 되니까 안 붙임.
	int32 AddItem(FName ItemID, int32 Count);	// 아이템 추가. 
	
	bool MoveItem(UBaruItemInstance* Item, FIntPoint NewTopLeft, bool bNewRotated);
	bool RemoveItem(UBaruItemInstance* Item, int32 Count);
	void UseItem(UBaruItemInstance* Item);

		// (4) 클라 호출(서버로 요청) -> 서버에서 실행되는 RPC 함수.
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveItem(UBaruItemInstance* Item, FIntPoint NewTopLeft); //아이템 방향 전환이 된다면 여기에 bool bNewRotated 넣을것.

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UseItem(UBaruItemInstance* Item);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_DropItem(UBaruItemInstance* Item, int32 Count);
	
	// 4. UI 갱신 부분.
	FOnInventoryUpdated OnInventoryUpdated;

		// 복제 콜백에서 호출 — Cells 재구성 : Cell - 슬롯의 각 구역?
	void RebuildCellCache();

private:
		// [진실원(진짜 부분)] 복제 대상
	UPROPERTY(Replicated)
	FInventorySlotArray SlotList;

		// [파생 캐시] GridWidth*GridHeight 크기. 복제하지 않음.
	UPROPERTY(Transient)
	TArray<TObjectPtr<UBaruItemInstance>> Cells;

		//좌표 X와 Y를 1차원 배열 인덱스로 바꿈.
		// 리플렉션으로는 배열 안의 배열을 다루기 까다롭고, 한 줄로 쭉 늘어진 형태가 메모리 성능으로도 이득이라서.
	FORCEINLINE int32 CellIndex(int32 X, int32 Y) const { return Y * GridWidth + X; }

	void OccupyCells(UBaruItemInstance* Item, FIntPoint TopLeft); // 아이템 차지 칸을 채움.
	void ClearCells(UBaruItemInstance* Item, FIntPoint TopLeft); // 아이템 차지 칸을 비움.

	FInventorySlot* FindSlot(const UBaruItemInstance* Item);
	const FItemData* FindItemData(FName ItemID) const;
	
	
	// [09.03. 추가]
		// 인벤토리 안의 Weapon Item을 Character의 EquipmentComponent에 장착.
		// 서버 내부에서만 호출.
	bool EquipWeaponItem(UBaruItemInstance* Item);

	friend struct FInventorySlotArray; // friend는 friend로 지정된 FInventorySlotArray만 RebuildCellCache 등의 private 함수를 쓸 수 있고, 접근 가능하게. 델타 복제 콜백 시.
};