// BaruBaseItem.cpp


#include "Gameplay/Items/BaruBaseItem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

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
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore); // Enum Collision Response
	CollisionComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);	// Enum Collision Channel
	
		//3. 외형 담당 - 메시. 메시는 콜리전 따로 안 함. 이미 콜리전을 따로 설정.
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(CollisionComponent);	// 콜리전 컴포넌트를 붙임. 충돌은 붙인 콜리전 담당.
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);	// 자체 콜리전은 No. 사용 안 함.
}

