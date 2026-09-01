// BaruBaseItem.h
// 아이템/인벤 제작순위 2. - 아이템의 기본 설정. 월드에 떨어질 아이템. 그러니 Actor.

// 아이템에 대한 기본 사항. 콜리전, 메시 등등.
// Grid-based Inventory도 BaseItem은 그대로.
//제작 순서 : 아이템 정보(데이터 테이블) -> 콜리전 부여 -> 그 콜리전을 보여줄 외형(콜리전과 외형 순서는 마음대로).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h" // 데이터 테이블을 쓰기 위해서 필요.
#include "Interfaces/InteractableInterface.h" // 상호작용 인터페이스. 엔진의 자체 클래스. Pick up용.
#include "BaruBaseItem.generated.h"

class USphereComponent; // 기본은 원형의 콜리전으로.
class UStaticMeshComponent; // 움직이는 아이템이면 나중에 SkeletalMesh도 고려. 액터에 Skeletal 넣으려면 추가 조치 필요.
class APawn; // PlayerState에서 인터렉트가 진행되므로, Pawn을 직접.

UCLASS()
class BARUGAME_API ABaruBaseItem : public AActor, public IInteractableInterface
{					//BaruBaseItem은 Actor이면서, Inter~도 가짐. Inter를 추가함으로, 상호작용도 가능하게 됨.
	GENERATED_BODY()
	
public:	
	ABaruBaseItem();
		// BeginPlay()와 Tick은 아이템에 필요 없음. 주기적 스폰이나 힐포션의 주기적 회복을 넣을 시 다른곳에서.

		//Item의 정보(아이디, 이름, 외형 등등. DataTable에 있는 정보)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "Item")
	FDataTableRowHandle ItemRow;	// 
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= "Item")
	USphereComponent* CollisionComponent;	// 콜리전 -> 스피어 형태로.
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category= "Item")
	UStaticMeshComponent* MeshComponent;	// Static Mesh는 다른데서 쓰일 수도 있으니까 이걸로 이름.
	
		//Item의 Pickup(습득)
		// 습득하는 아이템의 수량(총알 10발 등.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category= "Item", meta=(ClampMin = "1"))
	int32 PickupCount = 1;
	
		//습득 시도. 서버 전용 코드. 습득하는 아이템 전량을 수납 성공하면 자신을 파괴한 뒤, true 반환.
	UFUNCTION(BlueprintCallable, Category= "Item")
	bool TryPickup(AActor* Picker);
	
		// Interaction 구현. 시스템이 아이템에 물어보는 함수들. 캐릭터가 이 액터를 상호작용 대상으로 인식하고 호출.(260901 추가.)
	virtual bool CanInteract_Implementation(APawn* Interactor) const override;
	virtual FText GetInteractPromptText_Implementation(APawn* Interactor) const override;
	virtual FGameplayTag GetInteractionTag_Implementation() const override;
	virtual float GetInteractionDuration_Implementation() const override;
	virtual void ExecuteInteraction_Implementation(APawn* Interactor) override;
	
};
