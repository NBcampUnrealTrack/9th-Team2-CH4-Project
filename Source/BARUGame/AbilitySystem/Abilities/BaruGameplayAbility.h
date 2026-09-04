#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "BaruGameplayAbility.generated.h"

class APawn;
class AController;
class ABaruCharacter;
class UBaruAbilitySystemComponent;

// [Ability] 타입 정의
UENUM(BlueprintType)
enum class EBaruAbilityActivationPolicy : uint8
{
	OnInputTriggered    UMETA(DisplayName = "On Input Triggered"),
	WhileInputActive    UMETA(DisplayName = "While Input Active"),
	OnSpawn             UMETA(DisplayName = "On Spawn")
};

/**
 * 활성화 정책, 입력 태그 매핑 및 액터 접근자 제공
 */
UCLASS(Abstract)
class BARUGAME_API UBaruGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBaruGameplayAbility();

	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	
	EBaruAbilityActivationPolicy GetActivationPolicy() const { return ActivationPolicy; }
	
	FGameplayTag GetInputTag() const { return InputTag; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Ability")
	EBaruAbilityActivationPolicy ActivationPolicy = EBaruAbilityActivationPolicy::OnInputTriggered;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Ability")
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BARU|Ability|Feedback")
	FGameplayTag FailureFeedbackTag;

	// Helper
	UFUNCTION(BlueprintPure, Category = "BARU|Ability")
	APawn* GetAvatarPawnChecked() const;

	UFUNCTION(BlueprintPure, Category = "BARU|Ability")
	AController* GetControllerFromActorInfo() const;
	
	UFUNCTION(BlueprintPure, Category = "BARU|Ability")
	ABaruCharacter* GetBaruCharacterFromActorInfo() const;

	UFUNCTION(BlueprintPure, Category = "BARU|Ability")
	UBaruAbilitySystemComponent* GetBaruAbilitySystemComponentFromActorInfo() const;
};