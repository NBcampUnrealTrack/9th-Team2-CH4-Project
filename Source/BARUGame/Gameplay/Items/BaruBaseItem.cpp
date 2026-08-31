// BaruBaseItem.cpp


#include "Gameplay/Items/BaruBaseItem.h"
	//외형부분
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
	// Pick Up 부분.
#include "../Items/BaruBaseItem.h"
#include "Gameplay/Inventory/BaruInventoryComponent.h"	//습득한 "아이템"을 "인벤"으로 수납.
#include "GameFramework/Pawn.h"	// Pawn을 주우니까.
#include "GameFramework/PlayerState.h"	// PlayerState와 연결됨.

ABaruBaseItem::ABaruBaseItem()
{
	PrimaryActorTick.bCanEverTick = false;	// Tick 필요 없음.
	
		//1. 콜리전 크기, 형태 설정.
		// 목적 : 콜리전을 설정해서 습득 판정을 하기 위해.
		// LineTrace를 해도 바라보는 아이템과 상호작용할 수 있도록.
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(50.f);
	
		//2. 아이템을 보았을 때, LineTrace로 아이템을 인지하고, 상호작용이 가능하도록 하는 부분.
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // 쿼리 : 충돌만 / PhysicsOnly : 물리 계산 o / Query & Physics : 둘 다.
		// 우리조 지금 계획은 QueryOnly로. 이후 아이템과 어떤 작용 하는지에 따라 바뀔수도 있음.
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore); // Enum Collision Response, 모든 채널을 우선은 무시함. 
	CollisionComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);	// Enum Collision Channel, BaruCharacter.h에서 사용한 탐색전용 채널.
			// visibility를 Channel3으로 바꾼 이유 : Character에서 상호작용을 위해 쓴 전용 채널이라서.
	
		//3. 외형 담당 - 메시. 메시는 콜리전 따로 안 함. 이미 콜리전을 따로 설정.
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CollisionComponent);	// 콜리전 컴포넌트를 붙임. 충돌은 붙인 콜리전 담당.
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);	// 자체 콜리전은 No. 사용 안 함.
}

	//헬퍼. TryPickup 함수 이전, 인벤토리의 위치(PlayerState인지, Controller인지 등)를 찾는 역할.
namespace	// TryPickup에서만 사용되기에, 다른곳에서 사용하지 못하도록 namespace로 익명 지정.
{
	//
	UBaruInventoryComponent* FindInventoryOf(AActor* Actor)
	{
		if (!Actor) return nullptr;
		
		//1) 액터 자신
        if (UBaruInventoryComponent* Inv = Actor->FindComponentByClass<UBaruInventoryComponent>())
		{
			return Inv;
		}
		//
		if (APawn* Pawn = Cast<APawn>(Actor))
		{
			if (APlayerState* PS = Pawn->GetPlayerState())
			{
				return PS->FindComponentByClass<UBaruInventoryComponent>();
			}
		}
		return nullptr;
	}
}

	// 아이템 습득 TryPickup 실제 구현.
	// Character가 아니라 BaseItem에 있는 이유 : BaseItem이 아이템에 대한 정보(ID, 수량, 아이템 데이터들, 획득시 제거여부)를 갖고 있어서.
	// Character에서 직접 처리하면 아이템 내부규칙까지 모두 알게 되며, Character의 역할이 늘어나게 됨.
bool ABaruBaseItem::TryPickup(AActor* Picker)	// Actor : 타입 || Picker : 이름표. 함수를 호출할 때 넘겨받는 액터의 라벨.
{			//Picker : 줍는 주체의 매개변수. 실제로는 캐릭터(Pawn).
	if (!HasAuthority() || !Picker) return false;	// Authority가 아니거나, 줍지 않았다면 취소.
	
		// Picker에서 인벤토리 컴퍼넌트 찾기. -> PlayerState인지, 
	UBaruInventoryComponent* Inv = FindInventoryOf(Picker);	// 헬퍼 결과물의 실제 사용부.
	if (!Inv) return false;

	const int32 Left = Inv->AddItem(ItemRow.RowName, PickupCount);

		// 전량 수납 성공 시에만 파괴 (부분 수납이면 남은 만큼 월드에 유지)
	if (Left == 0)
	{
		Destroy();
		return true;
	}
		// 획득한 아이템이 일부만 인벤토리로 들어갔으면 남은 수량으로 갱신.
	PickupCount = Left;
	return false;
}