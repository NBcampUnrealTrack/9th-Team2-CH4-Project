

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
class ABaruMonsterDirector;

struct FOnAttributeChangeData;
struct FGameplayTag;

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
	
	// 사망 이외의 이유로 몬스터가 제거되면
	// GameMode의 몬스터 목록에서도 제거
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
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
	
	// DataAsset의 초기 능력치를 몬스터의 GAS Attribute에 적용
	void ApplyInitialAttributesFromDataAsset();
	
	UPROPERTY(
		ReplicatedUsing = OnRep_IsDead,
		VisibleInstanceOnly,
		BlueprintReadOnly,
		Category = "Monster|Combat"
	)
	bool bIsDead = false;
	
	UFUNCTION()
	void OnRep_IsDead();
	
	// 몬스터별 사망 애니메이션이나 래그돌 연출을
	// 자식 블루프린트에서 구현할 수 있도록 제공
	UFUNCTION(BlueprintImplementableEvent, Category = "Monster|Combat")
	void OnDeathCosmetic();
	
	// 그로기 태그가 추가되거나 제거됐을 때 호출
	void HandleGroggyTagChanged(
		const FGameplayTag Tag,
		int32 NewCount
	);

	// 서버에서 공격과 AI 행동을 멈추고 회복 타이머 시작
	void EnterGroggy();

	// 살아 있는 몬스터의 제압도를 회복하고 그로기 해제
	void RecoverFromGroggy();

	// 그로기 해제 후 살아 있는 몬스터의 행동 재개
	void ExitGroggy();
	
	// 평상시에도 다른 층으로 이동할 수 있는지
	// 맵 배치 몬스터는 false,
	// 이후 스포너에서 생성하는 몬스터는 true로 지정
	// 플레이어 추적과 이어지는 수색은 별도로 층 이동 허용
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Monster|AI",
		meta = (ExposeOnSpawn = "true")
	)
	bool bAllowIdleFloorTraversal = false;

	// 이 몬스터를 지휘할 디렉터
	// 지정하지 않으면 디렉터에 등록하지 않고 개별 AI로 행동
	UPROPERTY(
	EditInstanceOnly,
	BlueprintReadOnly,
	Category = "Monster|AI|Director",
	meta = (ExposeOnSpawn = "true")
)
	TObjectPtr<ABaruMonsterDirector> AssignedDirector;
	
	// 이 몬스터 종류가 로봇 디렉터의 명령을 받을지 결정
	// F0101은 블루프린트 기본값에서 활성화
	// 미행자는 비활성화 상태를 유지
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Monster|AI|Director"
	)
	bool bUsesMonsterDirector = false;
	
public:
	
	//이 몬스터가 사용하는 설정표를 반환
	const UBaruMonsterDataAsset* GetMonsterDataAsset() const;
	
	// 이 몬스터가 사용하는 ASC를 공통 인터페이스를 통해 반환
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	// 몬스터 사망 처리
	virtual void Die_Implementation(AActor* Killer) override;

	// 현재 사망 상태 반환
	virtual bool IsDead_Implementation() const override;
	
	// 체력 피해 처리 후 호출되는 피격 반응
	// 체력을 다시 감소시키지 않고 몬스터의 밀림만 처리
	virtual void ApplyCombatDamage_Implementation(
		float DamageAmount,
		const FHitResult& HitResult,
		AActor* DamageCauser,
		AController* InstigatedBy
	) override;
	
	// 디렉터와 AI가 평상시 층 이동 허용 여부를 확인
	bool CanTraverseFloorsWhileIdle() const
	{
		return bAllowIdleFloorTraversal;
	}
	
private:
	
	// 그로기 회복 예약을 관리
	// 사망하거나 월드에서 제거될 때 타이머 취소에 사용
	FTimerHandle GroggyRecoveryTimerHandle;

	// 그로기 태그 변경 알림의 연결 해제에 사용
	FDelegateHandle GroggyTagChangedHandle;

	// 같은 그로기 진입을 중복 처리하지 않도록 관리
	bool bIsGroggy = false;
	
};
