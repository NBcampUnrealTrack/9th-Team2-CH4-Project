#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "BaruAbilitySystemComponent.generated.h"

// UI or Feedback Delegate
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBaruAbilityActivationFailed, const UGameplayAbility*, Ability, const FGameplayTagContainer&, FailureTags);

/**
 * Tag 기반 입력 바인딩 처리 및 활성화 실패 태그 피드백 라우팅
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BARUGAME_API UBaruAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UBaruAbilitySystemComponent();
	
	// Ability Trigger Handler
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);
	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);
	
	virtual void NotifyAbilityFailed(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, const FGameplayTagContainer& FailureReasonTags) override;
	
	UFUNCTION(BlueprintCallable, Category = "BARU|GAS")
	bool TryActivateAbilityByTag(FGameplayTag AbilityTag);

public:
	// UI 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "BARU|GAS|Events")
	FOnBaruAbilityActivationFailed OnAbilityActivationFailed;

protected:
	// 이번 프레임에 눌리거나 유지 중인 입력 태그 목록(TArray)
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
};