#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Interfaces/CombatInterface.h" 
#include "BaruCharacter.generated.h"     

// 전방 선언 
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class USkeletalMeshComponent; //  Mesh1P를 위한 전방 선언

UCLASS()
class BARUGAME_API ABaruCharacter : public ACharacter, public ICombatInterface // [추가] ICombatInterface 상속
{
    GENERATED_BODY()

public:
    ABaruCharacter();
    
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    
    UFUNCTION(BlueprintCallable, Category = "BARU|Combat")
    bool PerformLineTrace(FHitResult& OutHitResult, float TraceDistance = 1000.0f);

    // 서버 상호작용 요청 RPC 예시
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_ProcessInteraction(const FHitResult& HitResult);
    
    virtual void PossessedBy(AController* NewController) override;
    virtual void OnRep_PlayerState() override;

    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void OnRep_Controller() override;
    
    // [추가] CombatInterface 구현 함수 
    virtual void Die_Implementation(AActor* Killer) override;
    virtual bool IsDead_Implementation() const override;

protected:
    virtual void BeginPlay() override;
    void InitAbilityActorInfo();

    // 이동 및 시점 회전 처리 함수
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);

protected:
    // 1인칭 카메라 
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Camera")
    TObjectPtr<UCameraComponent> FollowCamera;
    
    // [추가] 1인칭 팔/무기 전용 메쉬 (이게 없어서 cpp에서 에러 났던 것)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BARU|Mesh")
    TObjectPtr<USkeletalMeshComponent> Mesh1P;
    
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