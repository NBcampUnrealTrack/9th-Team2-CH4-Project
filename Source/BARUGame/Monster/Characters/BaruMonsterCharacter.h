

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaruMonsterCharacter.generated.h"

UCLASS()
class BARUGAME_API ABaruMonsterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	
	ABaruMonsterCharacter();

protected:
	
	virtual void BeginPlay() override;

public:
	
	
};
