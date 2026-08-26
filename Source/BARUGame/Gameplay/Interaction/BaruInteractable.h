// BaruInteractable.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BaruInteractable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType)
class UBaruInteractable : public UInterface
{
	GENERATED_BODY()
};


class BARUGAME_API IBaruInteractable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	// 서버에서 호출. 상호작용 실행.
	virtual void OnInteract(AActor* Interactor) = 0;

	// 화면에 띄울 문구 ("줍기", "문 열기")
	virtual FText GetInteractText() const { return FText::GetEmpty(); }
};


