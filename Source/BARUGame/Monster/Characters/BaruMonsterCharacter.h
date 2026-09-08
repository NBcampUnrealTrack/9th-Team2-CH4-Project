

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Interfaces/CombatInterface.h"
#include "BaruMonsterCharacter.generated.h"


class UBaruMonsterDataAsset;
class UAbilitySystemComponent;
class UBaruAbilitySystemComponent;
class UBaruCoreAttributeSet;
class UBaruMonsterAttributeSet;

struct FOnAttributeChangeData;

UCLASS()
class BARUGAME_API ABaruMonsterCharacter
	: public ACharacter, 
	  public IAbilitySystemInterface,
	  public ICombatInterface
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
	
	// 체력, 방어력, 이동속도처럼 모두가 사용하는 공용 수치
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Monster|GAS"
	)
	TObjectPtr<UBaruCoreAttributeSet> CoreAttributeSet;

	// 제압 게이지처럼 몬스터만 사용하는 수치
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Monster|GAS"
	)
	
	TObjectPtr<UBaruMonsterAttributeSet> MonsterAttributeSet;
	
	// CoreAttributeSet의 이동속도가 변경되면
	// CharacterMovement의 실제 최대속도에 반영
	void HandleMoveSpeedAttributeChanged(
		const FOnAttributeChangeData& AttributeChangeData
	);
	
	// DataAsset에 지정된 몬스터 Ability를 서버에서 ASC에 등록
	void GrantInitialAbilities();
	
	UPROPERTY(
		ReplicatedUsing = OnRep_IsDead,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Monster|Combat"
	)
	bool bIsDead = false;
	
	UFUNCTION()
	void OnRep_IsDead();

public:
	
	//이 몬스터가 사용하는 설정표를 반환
	const UBaruMonsterDataAsset* GetMonsterDataAsset() const;
	
	// 이 몬스터가 사용하는 ASC를 공통 인터페이스를 통해 반환
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	// 몬스터 사망 처리
	virtual void Die_Implementation(AActor* Killer) override;

	// 현재 사망 상태 반환
	virtual bool IsDead_Implementation() const override;
	
};
