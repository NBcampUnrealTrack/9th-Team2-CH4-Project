// BaruItemInstance.h
// 아이템/인벤 생성순위 3. - 아이템의 실제 저장 정보. 가방 안의 아이템. 실체가 없음. 그러니 Object.
				// Actor보다 가벼움.

// BaseItem과 차이 : 순수 데이터. 위치, 렌더링, 콜리전이 없는 Object. 인벤에 들어있을 아이템 정보.

//참고할 사항 : LyraInventoryItemInstance - 라이라의 태그 기반 수량/내구도 관리.
	// - 라이라의 인벤 전체를 가져오는 게 아니라, stat에 대한 내용만 참고만. 직접 라이라를 가져오진 않음.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BaruItemInstance.generated.h"

/**
 * 
 */
UCLASS(BlueprintType) // 나중에 WBP에서 블프 변수로 사용하려면.
class BARUGAME_API UBaruItemInstance : public UObject
{
	GENERATED_BODY()
	
public:
		//Actor보다 가벼워서 Object로 생성했지만, Object는 엔진에서 네트워크로 보낼 대상인지 자동으로 확인 못함.
		// Actor : 무비자 여행 가능. Object : 비자 필요.
		// 즉, 오브젝트만으로는 복제가 안 되니 Replicated Subobjects 기능 사용이 필요.
		//참고 : https://dev.epicgames.com/documentation/unreal-engine/replicating-uobjects-in-unreal-engine?lang=ko
	virtual bool IsSupportedForNetworking() const override { return true; } // 네트워크 대상이 될 자격 부여.(Like 비자.)
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutProps) const override; // 어떤 변수를 복제할지. -> cpp에서 고름.
	
	// DT_Items의 행 이름. 아이템의 데이터테이블에 넣을 항목.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item")
	FName ItemID;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item")
	int32 Quantity = 1;
	
	// 아이템 회전 기능 넣을 시, 이곳에 Rotation 관련 내용들 넣을 것.
};
