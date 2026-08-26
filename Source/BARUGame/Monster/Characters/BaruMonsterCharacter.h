

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "BaruMonsterCharacter.generated.h"


class UBaruMonsterDataAsset;
class UAbilitySystemComponent;
class UBaruAbilitySystemComponent;

UCLASS()
class BARUGAME_API ABaruMonsterCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	
	ABaruMonsterCharacter();

protected:
	
	virtual void BeginPlay() override;
	
	//몬스터 블루프린트에서 DataAsset을 선택
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Data")
	TObjectPtr<UBaruMonsterDataAsset> MonsterDataAsset;
	
	// 몬스터의 어빌리티, 효과, 상태 태그를 관리하는 ASC
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|GAS")
	TObjectPtr<UBaruAbilitySystemComponent> AbilitySystemComponent;

public:
	
	//이 몬스터가 사용하는 설정표를 반환
	const UBaruMonsterDataAsset* GetMonsterDataAsset() const;
	// 이 몬스터가 사용하는 ASC를 공통 인터페이스를 통해 반환
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	
};
