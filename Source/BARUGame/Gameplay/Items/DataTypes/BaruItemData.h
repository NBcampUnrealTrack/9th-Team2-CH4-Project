//BaruItemData.h
// 아이템/인벤 생성순위 1. 모든 아이템의 틀.
// 정적 스펙.
// 바닥에 떨어져 있을 때만 존재. -> 주운 뒤에는 ItemInstence에서.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h" // FTableRowBase 사용 목적.
#include "Net/Serialization/FastArraySerializer.h" // 직렬화. 네트워크 시, 슬롯 갱신 때 바뀐 것만 받기 위해. 하단에 다시 서술.
#include "BaruItemData.generated.h"



#pragma region 아이템에 넣어야할 내용 순서대로.
// 넣어야하는 것
	// 0. BaruItemInstance : 아이템 실체 부분을 아직 안 만들었음.
	// 1. [슬롯] 좌표 있는 슬롯 : Item 크기와 좌표를 대비해서 인벤의 슬롯에 넣을수 있는지 크기 판정.
	// 2. [배치] 2D 겹침 검사 : Item끼리 겹치는지 겹침 검사.
		// 아이템이 차지하는 영역을 크기 x 좌표 하여 다른 아이템과 겹치는지.
		// 아이템의 회전(bRotated)를 넣을지 이야기할 것.
		// 칸을 cells로 지정하고, width x height 크기의 1차원 배열 설정.
	// 3. [스택] 아이템 수량 쌓기 : bStackable - true : 쌓기 가능 / false : 불가.
	// 4. [복제] 서버 복제 : 인벤 내용을 클라로 복제하는 내용 넣기.
	// 5. 드래그 앤 드롭 - 복잡한 건 나중에.

#pragma endregion 


	// 전방선언 : BaseItem 과 텍스쳐는 포인터로만 가져올 것이니까.
class ABaruBaseItem;
class UTexture2D;
class UBaruItemInstance;	// 아이템 실체.
class UBaruInventoryComponent;	// 인벤 컴포넌트.

// 아이템 종류. 사용(UseItem)할 때 이 값으로 분기.
UENUM(BlueprintType)
enum class EItemType : uint8
{
	None,       // 설정 안 됨(기본값). 이 값이 보이면 DataTable에서 타입을 깜빡한 것.
		// 장비류
	Weapon,     // 무기 : 최우선 : 권총, 산탄 or 라이플. 두 종류.
	Bullet,		// 총알 : 소비템과 따로 설정할지는 구현하면서.
		// 획득 아이템들
	Package,	// 상자 : 가져갈 물품들.
	Goods,		// 상품 : 소비창에 보관 가능한 작은 물품들.
	Consumable, // 소비템 : 힐 포션 등등.
		// 수집물품
	Note        // 노트 : 수집 도감 목표. 구현 후순위.
};


// 아이템 하나에 들어가는 정보들. 데이터 테이블의 한 줄.
// 기존 강의와 다르게 grid-based Inventory(테트리스형 인벤)로 구현하기 때문에, 다른 구분 사항 필요할 것으로 보임.
USTRUCT(BlueprintType)
struct FItemData : public FTableRowBase
{
	GENERATED_BODY()
	
public:
	// 아이템 구분용 내부 ID : 내부식별용. - Gun, Note 등등.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	FName ItemID;
	
	// 화면상 띄워지는 아이템 이름 : 유저 식별용. - 피스톨, 수첩, 전깃줄 등등.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	FText ItemName;

	// 인벤토리 내의 2D 텍스쳐(썸네일)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	UTexture2D* Thumbnail = nullptr;
	
	// 아이템 종류. 사용할 때 어떤 동작을 할지 결정 (기본값 None = 아직 설정 안 됨)
	// 제일 상단의 Enum 참고.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	EItemType ItemType = EItemType::None;

	// 아이템의 설계도(클래스). 버리기(드롭)할 때 사용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	TSubclassOf<ABaruBaseItem> ItemActorClass;
	
#pragma region 그리드와 스택 부분들. 아이템 중첩하는 등.
		// 그리드 때문에 추가하는 부분들.
		// 아이템의 크기.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid", meta = (ClampMin = "1"))
	FIntPoint GridSize = FIntPoint(1, 1);
		// ClampMin : 최소값 지정. 방어용.
		// FIntPoint : 자료형. X, Y값 모두 int32. 칸 갯수는 정수라서 FVector2D가 아니라 IntPoint로.
	
	/* 아이템의 회전 기능. 회전해서 인벤에 넣을 수 있게 만들지 여부.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	bool bCanRotate = true;
	*/
	
		// 스택 쌓는 부분.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stack")
	bool bStackable = false; // 스택을 못 쌓게.
	
		// 최대 스택을 지정하는 변수.
		// 방어 코드. Stack을 쌓을 수 있는 경우, 최소 1개(Max 1로 방어).
		// ClampMin과 MaxStackSize는 방어코드. 최소값 0과 최대값 0이 안 되도록.
		// EditCondition = "bStackable" : bStackable일 때에만 이 부분을 에디터 UI에서 수정 가능.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stack",
		meta = (EditCondition = "bStackable", ClampMin = "1"))
	int32 MaxStackSize = 1;

#pragma endregion 
};


	// 배치 정보. 아이템 하나가 격자 어디에 놓였는지.
	// Serialize(직렬화) : 네트워크에서 데이터로 보낼 때, 메모리에 있는 구조체를 바이트 덩어리로 변환해서 전송하고, 받는 쪽에서 다시 구조체로 복원하는 과정.
	// FFastArraySerializerItem : "이 구조체는 FastArray 방식으로 복제될 배열의 원소"라고 표시하는 베이스 구조체.
USTRUCT()
struct FInventorySlot : public FFastArraySerializerItem
{
	GENERATED_BODY()
		
	UPROPERTY()
	TObjectPtr<UBaruItemInstance> Item = nullptr;
		
	// 아이템이 점유한 좌상단 격자의 좌표(시작점 좌표)
	UPROPERTY()
	FIntPoint TopLeft = FIntPoint::ZeroValue;		
};

	// 인벤 컴포넌트의 4. [복제]에 해당하는 부분.
	// [델타 복제 컨테이너] 변경된 슬롯만 전송
	// 델타(변경된 - Delta / Δ) 복제 : 이전 상태와 비교해서 변경된 속성만 네트워크로 전송하는 복제. 네트워크 전송 방식.
	// FastArraySerializer : 배열(Array)에서 변경·추가·삭제된 원소만 골라 델타 복제하고 싶을 때 사용하는 고성능 구조체. 더 빠르니 Fast.
	// 바뀐 원소, 즉 인벤에서 "포션 1개 사용"하면 인벤 전체가 아니라, 딱 그 바뀐 부분의 데이터만 전송.
	// <-> 일반적인 TArray<Replicated> : 원소 하나만 바뀌어도 배열 전체를 전송.
	// 원리는 아직 추가 확인 중.
USTRUCT()
struct FInventorySlotArray : public FFastArraySerializer
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<FInventorySlot> Slots; // 이건 FFastArray로 관리되고 있기 때문에 바뀐 슬롯만 델타 전송됨.
		//강의자료와 차이 - 강의에선 Slot을 FName ItemID만 넣고 끝내지만, 여기선 수량, 위치 등까지 고려.
	
	
	// 역참조 - 복제 대상 아님(NotReplicated)
	UPROPERTY(NotReplicated)
	TObjectPtr<UBaruInventoryComponent> OwnerComponent = nullptr;
	
	
#pragma region 서버 권한 인벤토리 처리 함수들 Inventory server operations

	// 클라에서만 호출되는 복제의 콜백. 클라로 복제 후 새로 더해지거나 변하거나, 복제 전에 삭제.
	// 선언은 ItemData에서 하고, 구현은 InventoryComponent.cpp에서 한 이유
		// 1. 이 함수들이 Inven~Comp~의 내부 상태(Cells, 델리게이트)를 조작하는 부분이라서.
		// 2. ItemData.cpp 생성 시 : ItemData가 Inven~Compo~.h를 Include해야하는 역전 형상 발생.
			//-> ItemData는 가장 기초적인 데이터 정의라서 아무도 못건들게 하기 위해서.
		// 복제 후 새로 더해지는 아이템들(슬롯이 추가).
	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
		// 복제 후 값이 변하는 템들.(수량 등)
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
		// 복제 전 제거되는 것들(복제로 제거되기 직전).
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);
	
		// 넷에서 델타 복제 시 직렬화 여부. 델타 복제의 실제 엔진.
		// FastArray~가 Slots 중 바뀐 원소만 골라서 직렬화.
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{		// 원소 타입은 FInventorySlot, 담는 배열 타입은 FInventorySlotArray 라고 엔진에 전달.
		return FastArrayDeltaSerialize<FInventorySlot, FInventorySlotArray>(Slots, DeltaParms, *this);
	}
	
#pragma endregion
	
	
};

	// 엔진이 네트워크에서 구조체를 복사할 때, 커스텀 델타 직렬화 기능(NetDeltaSerialize)을 가지고 있는지 확인하는 과정.
	// template<> : 다른 모든 타입은 범용 템플릿으로(기본값(false)으로) 둔 채, 특정 타입만 예외로 처리.
	// 즉, FInven~은 예외니(true), WithNet~ 함수를 복제해서 써도 됨 이라고 하는 것. 
template<>
	// template이 예외로 둔 특정 타입이 여기선 FInventorySlotArray.
struct TStructOpsTypeTraits<FInventorySlotArray> : public TStructOpsTypeTraitsBase2<FInventorySlotArray>
{
	enum { WithNetDeltaSerializer = true };
};