//BaruInventoryComponent.cpp



#include "BaruInventoryComponent.h"
#include "../Items/BaruItemInstance.h"
#include "../Items/BaruBaseItem.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"      // (SpawnActor, GetWorld, FActorSpawnParameters)
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h" // [추가] 컴포넌트 소유자가 PlayerState인지 확인
#include "GameFramework/Pawn.h"        // [추가] 실제 캐릭터 위치를 사용

UBaruInventoryComponent::UBaruInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;

	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;   // UE 5.1+
}

void UBaruInventoryComponent::InitializeComponent()
{
	Super::InitializeComponent();

	SlotList.OwnerComponent = this;
	Cells.SetNum(GridWidth * GridHeight);   // 전부 nullptr
}

void UBaruInventoryComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	if (IsUsingRegisteredSubObjectList())
	{
		for (const FInventorySlot& Slot : SlotList.Slots)
		{
			if (IsValid(Slot.Item))
			{
				AddReplicatedSubObject(Slot.Item);
			}
		}
	}
}

void UBaruInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 본인에게만 — 남의 가방을 보낼 이유가 없다 (대역폭 + 치팅 방지)
	FDoRepLifetimeParams Params;
	Params.Condition = COND_OwnerOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(UBaruInventoryComponent, SlotList, Params);
}

const FItemData* UBaruInventoryComponent::FindItemData(FName ItemID) const
{
	if (!ItemDataTable || ItemID.IsNone()) return nullptr;
	return ItemDataTable->FindRow<FItemData>(ItemID, TEXT("InventoryComponent"), false);
}


//3. 슬롯 판정과 인벤 내 2D 겹침 검사.
// 지정 좌표에 그 크기 아이템을 놓을 수 있는지.
// Ignore : 이동 중인 자기 자신은 겹침에서 제외 (자기 몸과 부딪혀 오판되는 것 방지).
bool UBaruInventoryComponent::IsRoomAvailable(FIntPoint TopLeft, FIntPoint Size,
	const UBaruItemInstance* Ignore) const
{
	// [1] 경계 검사 - 격자 밖으로 삐져나가면 실패.
	if (TopLeft.X < 0 || TopLeft.Y < 0) return false;
	if (TopLeft.X + Size.X > GridWidth)  return false;
	if (TopLeft.Y + Size.Y > GridHeight) return false;

	// [2] 겹침 검사 - Cells 캐시 덕분에 칸당 O(1).
	for (int32 Y = TopLeft.Y; Y < TopLeft.Y + Size.Y; ++Y)
	{
		for (int32 X = TopLeft.X; X < TopLeft.X + Size.X; ++X)
		{
			const UBaruItemInstance* Occupant = Cells[CellIndex(X, Y)];
			// 칸에 뭔가 있고, 그게 나 자신(Ignore)이 아니면 진짜 겹침.
			if (Occupant && Occupant != Ignore)
			{
				return false;
			}
		}
	}
	return true;
}

// 좌상단부터 가로 우선으로 훑어서 그 크기가 들어갈 첫 빈 자리를 찾음.
bool UBaruInventoryComponent::FindFirstFit(FIntPoint Size, FIntPoint& OutTopLeft) const
{
	for (int32 Y = 0; Y <= GridHeight - Size.Y; ++Y)
	{
		for (int32 X = 0; X <= GridWidth - Size.X; ++X)
		{
			if (IsRoomAvailable(FIntPoint(X, Y), Size))
			{
				OutTopLeft = FIntPoint(X, Y);   // 결과를 출력 인자에 담아서 돌려줌.
				return true;
			}
		}
	}
	return false;   // 어디에도 안 들어감.
}

// 특정 칸에 뭐가 있는지 조회. 없으면 nullptr.
UBaruItemInstance* UBaruInventoryComponent::GetItemAt(FIntPoint Cell) const
{
	if (Cell.X < 0 || Cell.Y < 0 || Cell.X >= GridWidth || Cell.Y >= GridHeight)
	{
		return nullptr;
	}
	return Cells[CellIndex(Cell.X, Cell.Y)];
}

// Cells 캐시 조작 (내부 헬퍼)
	// 아이템이 차지하는 칸들을 그 아이템으로 채움.
void UBaruInventoryComponent::OccupyCells(UBaruItemInstance* Item, FIntPoint TopLeft)
{
	// 회전 미구현이므로 현재는 정적 정의 크기를 그대로 사용.
	// 회전 넣을 시, Item->GetOccupiedSize() 같은 함수로 교체할 것.
	const FItemData* Data = FindItemData(Item->ItemID);
	const FIntPoint Size = Data ? Data->GridSize : FIntPoint(1, 1);

	for (int32 Y = TopLeft.Y; Y < TopLeft.Y + Size.Y; ++Y)
	for (int32 X = TopLeft.X; X < TopLeft.X + Size.X; ++X)
	{
		Cells[CellIndex(X, Y)] = Item;
	}
}

	// 아이템이 차지하던 칸들을 비움 (nullptr로).
void UBaruInventoryComponent::ClearCells(UBaruItemInstance* Item, FIntPoint TopLeft)
{
	const FItemData* Data = FindItemData(Item->ItemID);
	const FIntPoint Size = Data ? Data->GridSize : FIntPoint(1, 1);

	for (int32 Y = TopLeft.Y; Y < TopLeft.Y + Size.Y; ++Y)
	for (int32 X = TopLeft.X; X < TopLeft.X + Size.X; ++X)
	{
		// 혹시 다른 아이템이 차지 중이면 건드리지 않음 (안전장치).
		if (Cells[CellIndex(X, Y)] == Item)
		{
			Cells[CellIndex(X, Y)] = nullptr;
		}
	}
}

// SlotList(진실원)를 바탕으로 Cells(캐시)를 처음부터 다시 계산.
	// 복제로 SlotList가 갱신된 클라에서 콜백을 통해 호출됨.
void UBaruInventoryComponent::RebuildCellCache()
{
	Cells.Reset();
	Cells.SetNum(GridWidth * GridHeight);   // 전부 nullptr로 초기화.

	for (const FInventorySlot& Slot : SlotList.Slots)
	{
		if (IsValid(Slot.Item))
		{
			OccupyCells(Slot.Item, Slot.TopLeft);
		}
	}
}

	// 특정 아이템이 들어있는 슬롯을 SlotList에서 찾아 포인터 반환.
FInventorySlot* UBaruInventoryComponent::FindSlot(const UBaruItemInstance* Item)
{
	return SlotList.Slots.FindByPredicate(
		[Item](const FInventorySlot& S) { return S.Item == Item; });
}

// 4. 변경 - 서버 전용 (첫 줄에서 HasAuthority 체크)

	// 아이템 추가. 스택 가능하면 기존 스택부터 채우고, 남으면 새 칸에 배치.
	// @return 수납 못 한 잔여 수량 (0이면 전부 성공).
int32 UBaruInventoryComponent::AddItem(FName ItemID, int32 Count)
{
		// 서버가 아니면 실행 자체를 거부 (최후의 안전핀).
	if (!GetOwner()->HasAuthority() || Count <= 0) return Count;

	const FItemData* Data = FindItemData(ItemID);
	if (!Data)
	{
			// DT_Items에 해당 행이 없음 - 오타나 미등록 아이템.
		UE_LOG(LogTemp, Warning, TEXT("AddItem 실패: '%s' 행 없음"), *ItemID.ToString());
		return Count;
	}

	int32 Remaining = Count;

		// [스택] 쌓기 가능한 아이템만. 불가면 이 블록을 통째로 건너뜀.
	if (Data->bStackable)
	{
		for (FInventorySlot& Slot : SlotList.Slots)
		{
			if (Remaining <= 0) break;
			if (!IsValid(Slot.Item) || Slot.Item->ItemID != ItemID) continue;

			const int32 Space = Data->MaxStackSize - Slot.Item->Quantity;
			if (Space <= 0) continue;   // 이미 꽉 찬 스택.

			const int32 Added = FMath::Min(Space, Remaining);
			Slot.Item->Quantity += Added;
			Remaining -= Added;

			SlotList.MarkItemDirty(Slot);   // 이 슬롯만 델타 전송.
		}
	}

		// [배치] 남은 수량을 새 칸에 배치.
	while (Remaining > 0)
	{
		FIntPoint TopLeft;
			// 회전 미구현이므로 정방향 한 번만 시도. 실패하면 그대로 종료.
		if (!FindFirstFit(Data->GridSize, TopLeft))
		{
			break;   // 공간 부족 - 잔여 수량 반환.
		}

		UBaruItemInstance* NewItem = NewObject<UBaruItemInstance>(GetOwner());
		NewItem->ItemID   = ItemID;
		NewItem->Quantity = Data->bStackable
			? FMath::Min(Data->MaxStackSize, Remaining)
			: 1;

		FInventorySlot& NewSlot = SlotList.Slots.AddDefaulted_GetRef();
		NewSlot.Item    = NewItem;
		NewSlot.TopLeft = TopLeft;

		OccupyCells(NewItem, TopLeft);
		SlotList.MarkItemDirty(NewSlot);

			// 새로 만든 UObject를 복제 목록에 등록 (안 하면 클라에서 null).
		if (IsUsingRegisteredSubObjectList() && IsReadyForReplication())
		{
			AddReplicatedSubObject(NewItem);
		}

		Remaining -= NewItem->Quantity;
	}

	OnInventoryUpdated.Broadcast();   // UI 갱신 방송.
	return Remaining;
}

	// 아이템 이동. 실패 시 원위치로 롤백.
bool UBaruInventoryComponent::MoveItem(UBaruItemInstance* Item, FIntPoint NewTopLeft, bool bNewRotated)
{
	if (!GetOwner()->HasAuthority() || !IsValid(Item)) return false;

	FInventorySlot* Slot = FindSlot(Item);
	if (!Slot) return false;

	const FIntPoint OldTopLeft = Slot->TopLeft;

		// 자기 자신을 먼저 비워야 겹침 오판이 없음.
	ClearCells(Item, OldTopLeft);

	const FItemData* Data = FindItemData(Item->ItemID);
	const FIntPoint Size = Data ? Data->GridSize : FIntPoint(1, 1);

		// 새 위치가 안 되면 원래 자리로 되돌리고 실패.
	if (!IsRoomAvailable(NewTopLeft, Size))
	{
		OccupyCells(Item, OldTopLeft);   // 롤백.
		return false;
	}

	Slot->TopLeft = NewTopLeft;
	OccupyCells(Item, NewTopLeft);
	SlotList.MarkItemDirty(*Slot);

	OnInventoryUpdated.Broadcast();
	return true;
}

	// 아이템 제거. 수량이 남으면 부분 차감, 다 빠지면 슬롯 통째 제거.
bool UBaruInventoryComponent::RemoveItem(UBaruItemInstance* Item, int32 Count)
{
	if (!GetOwner()->HasAuthority() || !IsValid(Item) || Count <= 0) return false;

	const int32 Index = SlotList.Slots.IndexOfByPredicate(
		[Item](const FInventorySlot& S) { return S.Item == Item; });
	if (Index == INDEX_NONE) return false;

	FInventorySlot& Slot = SlotList.Slots[Index];

		// 부분 차감 - 요청량보다 많이 갖고 있으면 수량만 줄임.
	if (Item->Quantity > Count)
	{
		Item->Quantity -= Count;
		SlotList.MarkItemDirty(Slot);
		OnInventoryUpdated.Broadcast();
		return true;
	}

		// 전량 제거.
	ClearCells(Item, Slot.TopLeft);

		// 복제 목록에서 등록 해제 (반드시 - 안 하면 GC 후 크래시 위험).
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication())
	{
		RemoveReplicatedSubObject(Item);
	}

	SlotList.Slots.RemoveAt(Index);
	SlotList.MarkArrayDirty();   // 배열 구조가 바뀌었으니 전체 표시.

	OnInventoryUpdated.Broadcast();
	return true;
}

	// 아이템 사용. 타입별로 분기 (회복 등). 사용 후 1개 소모.
void UBaruInventoryComponent::UseItem(UBaruItemInstance* Item)
{
	if (!GetOwner()->HasAuthority() || !IsValid(Item)) return;

	const FItemData* Data = FindItemData(Item->ItemID);
	if (!Data) return;

		// 타입별 효과 분기. 지금은 뼈대만 - 실제 효과는 팀 로직 붙일 때 채움.
	switch (Data->ItemType)
	{
	case EItemType::Consumable:
		// TODO: 회복 등 소비 효과. 예) 소유 캐릭터 Heal 호출.
		break;
	default:
			// 그 외 타입은 아직 사용 효과 없음.
		return;
	}

		// 효과를 준 뒤 1개 소모.
	RemoveItem(Item, 1);
}



// 5. RPC - 클라 요청을 서버가 받아 검증 후 실행

void UBaruInventoryComponent::Server_MoveItem_Implementation(UBaruItemInstance* Item, FIntPoint NewTopLeft)
{
	MoveItem(Item, NewTopLeft, false);   // 회전 미구현이므로 false 고정.
}

	// 명백히 조작된 좌표는 여기서 커넥션 차단.
bool UBaruInventoryComponent::Server_MoveItem_Validate(UBaruItemInstance* Item, FIntPoint NewTopLeft)
{
	return NewTopLeft.X >= 0 && NewTopLeft.Y >= 0
		&& NewTopLeft.X < GridWidth && NewTopLeft.Y < GridHeight;
}

void UBaruInventoryComponent::Server_UseItem_Implementation(UBaruItemInstance* Item)
{
		// 남의 아이템을 조작하는 요청 차단 - 내 슬롯에 있는 것만 허용.
	if (!IsValid(Item) || !FindSlot(Item)) return;
	UseItem(Item);
}

bool UBaruInventoryComponent::Server_UseItem_Validate(UBaruItemInstance* Item)
{
	return true;
}

void UBaruInventoryComponent::Server_DropItem_Implementation(UBaruItemInstance* Item, int32 Count)
{
	if (!IsValid(Item) || !FindSlot(Item)) return;

	const FItemData* Data = FindItemData(Item->ItemID);
	if (!Data || !Data->ItemActorClass) return;

	const int32 Actual = FMath::Min(Count, Item->Quantity);

		// [수정:260901] 인벤토리의 소유자가 PlayerState.
		// PlayerState가 아닌 실제 플레이어 Pawn(Character) 앞에 아이템을 드롭.
	AActor* SpawnRef = GetOwner();	//Spawn Reference -> SpawnRef : 스폰 출처. 생성된 액터의 참조변수의 줄임말.

	if (const APlayerState* OwnerPS = Cast<APlayerState>(GetOwner()))
	{
		if (APawn* OwnerPawn = OwnerPS->GetPawn())
		{
			SpawnRef = OwnerPawn;
		}
	}

		// 소유자나 Pawn을 찾지 못한 비정상 상황에서는 드롭 안 함.
	if (!IsValid(SpawnRef))
	{
		return;
	}

	const FVector SpawnLoc =
		SpawnRef->GetActorLocation()
		+ SpawnRef->GetActorForwardVector() * 100.f;

		//아이템 액터를 생성할 때 사용할 설정상자 Params를 생성.
	FActorSpawnParameters Params; //액터(Actor)를 스폰(Spawn)할 때 필요한 설정들을 담은(Param~) 구조체(F), 줄여서 변수명 Params.
	Params.SpawnCollisionHandlingOverride =	// Spawn~ : 생성 위치에 충돌물이 있을 때 어떻게 할지 정하는 항목. world.h 에 구현.
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;	// EngineTypes.h에 구현.
											//“원래 위치에 충돌물이 있으면, 근처의 가능한 위치로 조금 조정해 본다. 조정에 실패해도 Actor 생성 자체는 취소하지 말고 생성한다.”
				//아이템 드롭 위치에 문제 있으면 다른 걸로 바꾸면 됨. AdjustIfPossibleButDontSpawnIfColliding. 로 하면 충돌시, 액터 생성 안 함.
	
		// 스폰 성공을 확인한 뒤에만 슬롯 제거 (데이터가 사라지는 순서로 일하지 않음).
	ABaruBaseItem* Spawned = GetWorld()->SpawnActor<ABaruBaseItem>(
		Data->ItemActorClass, SpawnLoc, FRotator::ZeroRotator, Params);

	if (Spawned)
	{
		RemoveItem(Item, Actual);
	}
}

bool UBaruInventoryComponent::Server_DropItem_Validate(UBaruItemInstance* Item, int32 Count)
{
	return Count > 0;
}

// FastArray 콜백 - 클라에서 SlotList 복제 수신 시 자동 호출됨
	// 슬롯이 새로 추가됨 -> 캐시 재구성 + UI 갱신.
void FInventorySlotArray::PostReplicatedAdd(const TArrayView<int32>&, int32)
{
	if (OwnerComponent)
	{
		OwnerComponent->RebuildCellCache();
		OwnerComponent->OnInventoryUpdated.Broadcast();
	}
}

	// 슬롯 값이 바뀜(수량/위치 등) -> 캐시 재구성 + UI 갱신.
void FInventorySlotArray::PostReplicatedChange(const TArrayView<int32>&, int32)
{
	if (OwnerComponent)
	{
		OwnerComponent->RebuildCellCache();
		OwnerComponent->OnInventoryUpdated.Broadcast();
	}
}

	// 슬롯이 제거되기 직전 -> UI 갱신 (캐시는 다음 Add/Change에서 재구성).
void FInventorySlotArray::PreReplicatedRemove(const TArrayView<int32>&, int32)
{
	if (OwnerComponent)
	{
		OwnerComponent->OnInventoryUpdated.Broadcast();
	}
}