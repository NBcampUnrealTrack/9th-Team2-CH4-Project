// BaruInventoryComponent.h
// 인벤토리 코드의 메인.
// 강의와 다르게 그리드 인벤으로.
// 참고자료 : cpp - https://github.com/imnazake/grid-inventory-sample
// 블루프린트 - https://murlocdev.tistory.com/40

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Gameplay/Items/DataTypes/BaruItemData.h"   // FInventorySlot, FInventorySlotArray 사용 목적. 실제 경로에 맞게 수정.
#include "Gameplay/Inventory/DataTypes/BaruInventoryNotification.h"
#include "BaruInventoryComponent.generated.h"

class UDataTable;
class UBaruItemInstance;

DECLARE_MULTICAST_DELEGATE(FOnInventoryUpdated); // UI(태현님)와 맞춰서 이름 정할 것.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnBaruInventoryPickupResult,
	const FBaruInventoryPickupNotification&,
	Notification);

// 현재 인벤토리로부터 계산한 정산 결과입니다.
// 계산 결과만 담으며 인벤토리 아이템을 제거하거나 변경하지 않습니다.
USTRUCT(BlueprintType)
struct BARUGAME_API FBaruInventorySettlementSummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BARU|Inventory|Settlement")
	int32 EligibleItemUnitCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BARU|Inventory|Settlement")
	int32 TotalValue = 0;
};

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
	int32 GridWidth = 7;

	UPROPERTY(EditAnywhere, Category = "Grid", meta = (ClampMin = "1"))
	int32 GridHeight = 10;
	
		// DT_Items 할당
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	UDataTable* ItemDataTable = nullptr;	
	
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

	// [추가] 월드 아이템 습득 전용 진입점입니다.
	// AddItem과 결과 계산·소유 클라이언트 전달을 InventoryComponent 안에서 끝냅니다.
	int32 AddPickupItem(FName ItemID, int32 Count);
	
	bool MoveItem(UBaruItemInstance* Item, FIntPoint NewTopLeft, bool bNewRotated);
	bool RemoveItem(UBaruItemInstance* Item, int32 Count);
	void UseItem(UBaruItemInstance* Item);
	
	// 장착 시 격자 점유를 해제하고,
	// 해제 시 첫 빈 위치에 다시 배치.
	// ItemInstance와 복제 등록은 유지.
	bool SetItemEquipped(
		UBaruItemInstance* Item,
		bool bNewEquipped);

	bool IsItemEquipped(
		const UBaruItemInstance* Item) const;
	
		// 로컬 플레이어의 아이템 사용 요청을
		// 서버 권한 UseItem으로 전달합니다.
	UFUNCTION(BlueprintCallable, Category = "BARU|Inventory")
	void RequestUseItem(UBaruItemInstance* Item);

		// (4) 클라 호출(서버로 요청) -> 서버에서 실행되는 RPC 함수.
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveItem(UBaruItemInstance* Item, FIntPoint NewTopLeft); //아이템 방향 전환이 된다면 여기에 bool bNewRotated 넣을것.

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UseItemAtCell(
		FIntPoint ItemCell,
		FName ExpectedItemID);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_DropItem(UBaruItemInstance* Item, int32 Count);
	
	// 4. UI 갱신 부분.
	FOnInventoryUpdated OnInventoryUpdated;

	// [추가] 소유 클라이언트에서만 Broadcast되는 획득 결과입니다.
	UPROPERTY(BlueprintAssignable, Category = "BARU|Inventory|Pickup")
	FOnBaruInventoryPickupResult OnInventoryPickupResult;

	// 서버가 확정한 결과를 이 InventoryComponent의 소유 클라이언트에 전달합니다.
	UFUNCTION(Client, Reliable)
	void Client_ReceiveInventoryPickupResult(
		const FBaruInventoryPickupNotification& Notification);

		// 복제 콜백에서 호출 — Cells 재구성 : Cell - 슬롯의 각 구역?
	void RebuildCellCache();
	
		// 인벤 이동 요청
	UFUNCTION(BlueprintCallable, Category = "BARU|Inventory")
	void RequestMoveItem(
		UBaruItemInstance* Item,
		FIntPoint NewTopLeft);
	
		// 장착 중인 아이템을 지정한 Grid 좌표에 반환.
		// 서버에서만 실행.
	bool ReturnEquippedItemToCell(
		UBaruItemInstance* Item,
		FIntPoint NewTopLeft);
	
	
	// Seamless Travel 시 이전 Inventory에서 슬롯 데이터를 복사합니다.
	// 서버 내부에서만 호출합니다.
	void CopyInventoryFrom(
		const UBaruInventoryComponent* SourceInventory);
	
	// Grid 보관품과 장착품을 합쳐 계산합니다. 같은 아이템을 중복 합산하지 않습니다.
	UFUNCTION(BlueprintPure, Category = "BARU|Inventory|Weight")
	float GetTotalCarriedWeightKg() const;

		// Grid 보관품과 장착품을 모두 대상으로 정산 가능 수량과 가치를 계산.
		// 서버/클라이언트 어느 쪽에서도 읽을 수 있지만 실제 보상 확정은 서버 GameMode가 수행 필요.
	UFUNCTION(BlueprintPure, Category = "BARU|Inventory|Settlement")
	FBaruInventorySettlementSummary CalculateSettlementSummary() const;
	
	// UI용: 해당 인벤토리 슬롯의 수량 전체를 버리는 요청.
	UFUNCTION(BlueprintCallable, Category = "BARU|Inventory")
	void RequestDropEntireItem(UBaruItemInstance* Item);
	
		// 장착품을 포함한 전체 보유 아이템의 Quantity 합계.
		// 정산 대상 여부나 가격과 관계없이 수량만 계산.
	UFUNCTION(BlueprintPure, Category = "BARU|Inventory")
	int32 GetTotalItemCount() const;

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

	void SendPickupResultToOwner(
		FName ItemID,
		int32 RequestedQuantity,
		int32 RemainingQuantity);

	FInventorySlot* FindSlot(const UBaruItemInstance* Item);
	const FItemData* FindItemData(FName ItemID) const;
	
	
	// [09.03. 추가]
		// 인벤토리 안의 Weapon Item을 Character의 EquipmentComponent에 장착.
		// 서버 내부에서만 호출.
	bool EquipWeaponItem(UBaruItemInstance* Item);

	friend struct FInventorySlotArray; // friend는 friend로 지정된 FInventorySlotArray만 RebuildCellCache 등의 private 함수를 쓸 수 있고, 접근 가능하게. 델타 복제 콜백 시.
	
	bool DropEntireItemOnServer(UBaruItemInstance* Item, int32 ExpectedQuantity);
};
