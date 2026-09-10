

#include "Monster/Navigation/BaruNavAreaFloorTransition.h"

UBaruNavAreaFloorTransition::UBaruNavAreaFloorTransition()
{
	// 추적·수색 또는 자유 이동 몬스터는 일반 비용으로 통과
	DefaultCost = 1.0f;

	// 에디터에서 층 연결 구간을 구분하기 위한 색상
	DrawColor = FColor(255, 140, 0);
}