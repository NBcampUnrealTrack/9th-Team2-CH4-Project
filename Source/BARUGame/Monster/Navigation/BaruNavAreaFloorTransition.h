

#pragma once

#include "CoreMinimal.h"
#include "NavAreas/NavArea.h"
#include "BaruNavAreaFloorTransition.generated.h"


// 계단과 경사로 등 층을 연결하는 이동 구간을 표시
// 영역 자체는 통행 가능하며, 개체별 경로 필터에서 통행 여부를 결정
UCLASS()
class BARUGAME_API UBaruNavAreaFloorTransition : public UNavArea
{
	GENERATED_BODY()
	
public:
	UBaruNavAreaFloorTransition();
	
};
