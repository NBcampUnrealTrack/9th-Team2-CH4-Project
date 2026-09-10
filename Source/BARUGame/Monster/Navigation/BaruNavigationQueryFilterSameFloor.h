

#pragma once

#include "CoreMinimal.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "BaruNavigationQueryFilterSameFloor.generated.h"


// 층 연결 구간을 사용하지 않는 경로 필터
// 맵 배치 몬스터의 평상시 이동에 적용
UCLASS()
class BARUGAME_API UBaruNavigationQueryFilterSameFloor : public UNavigationQueryFilter
{
	GENERATED_BODY()
	
public:
	UBaruNavigationQueryFilterSameFloor();
};
