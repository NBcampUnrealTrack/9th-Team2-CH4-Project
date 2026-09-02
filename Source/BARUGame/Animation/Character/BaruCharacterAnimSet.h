#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BaruCharacterAnimSet.generated.h"

class UAnimMontage;

/**
 * 캐릭터가 재생하는 몽타주 모음.
 * C++ 에 몽타주를 직접 들고 있지 않는 이유:
 * 애니메이션 교체는 애니메이터가 자주 하는 작업인데,
 * C++ 변수로 두면 교체할 때마다 코드 수정 + 빌드가 필요해집니다.
 * (README 4장 3번 Data-Driven Design)
 */
UCLASS(BlueprintType)
class BARUGAME_API UBaruCharacterAnimSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 사망 몽타주. 여러 개 넣으면 그중 하나가 무작위로 재생됩니다.
	// 비워두면 몽타주 없이 래그돌만 사용합니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Anim|Death")
	TArray<TObjectPtr<UAnimMontage>> DeathMontages;

	// 피격 리액션 몽타주.
	// Todo: 피격 방향(전/후/좌/우)과 강도(약/중/강)로 나눌지는 전투 사양 확정 후 결정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Anim|HitReact")
	TArray<TObjectPtr<UAnimMontage>> HitReactMontages;

	// 배열에서 무작위로 하나 뽑습니다. 비어있으면 nullptr.
	// BP 에서 매번 Random Array Item + Is Valid 를 엮지 않도록 C++ 에 둡니다.
	UFUNCTION(BlueprintPure, Category = "BARU|Anim")
	UAnimMontage* GetRandomDeathMontage() const;

	UFUNCTION(BlueprintPure, Category = "BARU|Anim")
	UAnimMontage* GetRandomHitReactMontage() const;
};