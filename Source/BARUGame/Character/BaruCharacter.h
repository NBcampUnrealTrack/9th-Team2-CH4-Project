#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "BaruCharacter.generated.h"

// 전방 선언 
class UCameraComponent;
class UInputMappingContext;
class UInputAction;

UCLASS()
class BARUGAME_API ABaruCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABaruCharacter();
    
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void InitAbilityActorInfo();
	// 이동 및 시점 회전 처리 함수
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

protected:
	//  1인칭 카메라 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
	// 인풋 액션 에셋 할당용 포인터 변수 
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
	TObjectPtr<UInputAction> JumpAction;
};