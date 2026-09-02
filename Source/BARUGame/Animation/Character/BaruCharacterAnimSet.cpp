#include "Animation/Character/BaruCharacterAnimSet.h"
#include "Animation/AnimMontage.h"

namespace
{
	// 두 함수가 똑같은 일을 하므로 헬퍼로 묶습니다.
	UAnimMontage* PickRandom(const TArray<TObjectPtr<UAnimMontage>>& Montages)
	{
		if (Montages.Num() == 0)
		{
			return nullptr;
		}

		// 배열에 빈 칸(None)이 섞여 있어도 크래시 나지 않도록 유효한 것만 모읍니다.
		TArray<UAnimMontage*> Valid;
		Valid.Reserve(Montages.Num());
		for (const TObjectPtr<UAnimMontage>& M : Montages)
		{
			if (IsValid(M))
			{
				Valid.Add(M);
			}
		}

		return Valid.Num() > 0 ? Valid[FMath::RandRange(0, Valid.Num() - 1)] : nullptr;
	}
}

UAnimMontage* UBaruCharacterAnimSet::GetRandomDeathMontage() const
{
	return PickRandom(DeathMontages);
}

UAnimMontage* UBaruCharacterAnimSet::GetRandomHitReactMontage() const
{
	return PickRandom(HitReactMontages);
}