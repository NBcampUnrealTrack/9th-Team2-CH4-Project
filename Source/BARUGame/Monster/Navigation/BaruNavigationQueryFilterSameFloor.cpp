


#include "Monster/Navigation/BaruNavigationQueryFilterSameFloor.h"
#include "Monster/Navigation/BaruNavAreaFloorTransition.h"

UBaruNavigationQueryFilterSameFloor::
	UBaruNavigationQueryFilterSameFloor()
{
	FNavigationFilterArea FloorTransitionArea;

	// 층 연결 구간을 필터 대상으로 지정
	FloorTransitionArea.AreaClass =
		UBaruNavAreaFloorTransition::StaticClass();

	// 비용만 높이는 것이 아니라 경로 후보에서 제외
	FloorTransitionArea.bIsExcluded = true;

	Areas.Add(FloorTransitionArea);
}