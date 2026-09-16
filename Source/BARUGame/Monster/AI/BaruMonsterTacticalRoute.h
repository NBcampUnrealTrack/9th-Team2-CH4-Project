

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaruMonsterTacticalRoute.generated.h"

class ABaruMonsterCharacter;
class USplineComponent;


UCLASS()
class BARUGAME_API ABaruMonsterTacticalRoute : public AActor
{
	GENERATED_BODY()

public:
	ABaruMonsterTacticalRoute();

	// 우회 이동을 시작할 첫 번째 스플라인 지점
	UFUNCTION(
		BlueprintPure,
		Category = "Monster|AI|Tactical Route"
	)
	FVector GetRouteEntryLocation() const;

	// 최종적으로 퇴로를 막을 마지막 스플라인 지점
	UFUNCTION(
		BlueprintPure,
		Category = "Monster|AI|Tactical Route"
	)
	FVector GetBlockLocation() const;

	// 경로에 사용할 수 있는 지점이 올바르게 배치됐는지 확인
	UFUNCTION(
		BlueprintPure,
		Category = "Monster|AI|Tactical Route"
	)
	bool IsRouteConfigured() const;

	// 지정한 몬스터가 이 경로를 사용할 수 있는지 확인
	bool IsAvailableFor(
		const ABaruMonsterCharacter* Monster
	) const;

	// 한 경로에 여러 몬스터가 몰리지 않도록 경로 예약
	bool TryReserve(
		ABaruMonsterCharacter* Monster
	);

	// 우회 명령 종료 후 경로 예약 해제
	void ReleaseReservation(
		const ABaruMonsterCharacter* Monster
	);

protected:

	// 첫 점은 우회 경유지,
	// 마지막 점은 최종 차단 위치로 사용
	UPROPERTY(
		VisibleAnywhere,
		BlueprintReadOnly,
		Category = "Monster|AI|Tactical Route"
	)
	TObjectPtr<USplineComponent> RouteSpline;

	// 비활성화하면 디렉터가 이 경로를 사용하지 않음
	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		Category = "Monster|AI|Tactical Route"
	)
	bool bRouteEnabled = true;

private:

	// 현재 이 경로를 사용 중인 몬스터
	// 몬스터가 제거되면 자동으로 무효화되는 약한 참조
	UPROPERTY(Transient)
	TWeakObjectPtr<ABaruMonsterCharacter> ReservedBy;
};
