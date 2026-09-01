//BaruItemInstance.cpp


#include "Gameplay/Items/BaruItemInstance.h"
#include "Net/UnrealNetwork.h"


void UBaruItemInstance::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
		// 네트워크로 복제할 변수들.
	
		// 인벤토리 아이템 인스턴스의 실제 값을 네트워크로 복제할 변수를 등록.
	DOREPLIFETIME(UBaruItemInstance, ItemID);
	DOREPLIFETIME(UBaruItemInstance, Quantity);
	
}