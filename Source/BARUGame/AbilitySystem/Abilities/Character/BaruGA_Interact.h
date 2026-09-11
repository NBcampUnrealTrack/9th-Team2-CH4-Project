#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/BaruGameplayAbility.h"
#include "BaruGA_Interact.generated.h"

// ==============================================================================
// [Deprecated / Dormant Notice]
// 현재 인게임 상호작용(아이템 습득, 문 조작, DBNO 소생 등)은 ABaruCharacter의
// Server_ProcessInteraction RPC 및 IInteractableInterface 직접 호출 경로로 일원화되어 있습니다.
// 향후 상호작용 파이프라인을 GAS(TargetData / GameplayAbilityTargetActor)로 
// 전면 마이그레이션할 때 참고/재활성화하기 위해 코드를 보존합니다.
// ==============================================================================

UCLASS(Abstract, meta = (DisplayName = "[Dormant] Baru GA Interact"))
class BARUGAME_API UBaruGA_Interact : public UBaruGameplayAbility
{
	GENERATED_BODY()

public:
	UBaruGA_Interact();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	void TickInteractCheck();
	AActor* PerformTrace();
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "BARU|Interaction")
	float TraceDistance = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category = "BARU|Interaction")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_GameTraceChannel3; // Interaction
	
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentTargetActor;

	float CurrentHoldTime = 0.0f;
	float RequiredDuration = 0.0f;

	FTimerHandle HoldTimerHandle;
};