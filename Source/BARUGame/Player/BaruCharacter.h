#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "BaruCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;

UCLASS()
class BARUGAME_API ABaruCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABaruCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// 이동 및 시점 회전 처리 함수
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

protected:
	// 3인칭 카메라 지지대
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Camera")
	USpringArmComponent* CameraBoom;

	// 3인칭 카메라
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Camera")
	UCameraComponent* FollowCamera;

	// 인풋 액션 에셋 할당용 포인터 변수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
	UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
	UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Input")
	UInputAction* JumpAction;
};