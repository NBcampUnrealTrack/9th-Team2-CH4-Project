

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BaruMonsterCharacter.generated.h"


class UBaruMonsterDataAsset;

UCLASS()
class BARUGAME_API ABaruMonsterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	
	ABaruMonsterCharacter();

protected:
	
	virtual void BeginPlay() override;
	
	//몬스터 블루프린트에서 DataAsset을 선택
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Data")
	TObjectPtr<UBaruMonsterDataAsset> MonsterDataAsset;

public:
	
	//이 몬스터가 사용하는 설정표를 반환
	const UBaruMonsterDataAsset* GetMonsterDataAsset() const;

	
	
};
