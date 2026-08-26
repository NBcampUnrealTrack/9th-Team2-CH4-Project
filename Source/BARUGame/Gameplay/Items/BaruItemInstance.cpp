//BaruItemInstance.cpp


#include "Gameplay/Items/BaruItemInstance.h"
#include "Net/UnrealNetwork.h"


void UBaruItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutProps) const
{
	Super::GetLifetimeReplicatedProps(OutProps);
		// 네트워크로 복제할 변수들.
	/*
	DOREPLIFETIME(UBaruItemInstance, ItemID);
	DOREPLIFETIME(UBaruItemInstance, Quantity);
	*/
}