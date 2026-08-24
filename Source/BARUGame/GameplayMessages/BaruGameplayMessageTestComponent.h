// BaruGameplayMessageTestComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"

#include "BaruGameplayMessageTestComponent.generated.h"

struct FBaruDebugMessage;

/**
 * Gameplay Message Subsystem의 등록, 발행, 수신, 해제를 학습하기 위한 Component
 */
UCLASS(ClassGroup = (BARU), meta = (BlueprintSpawnableComponent))
class BARUGAME_API UBaruGameplayMessageTestComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UBaruGameplayMessageTestComponent();
	
	/**
	 * Message.Debug.Test 채널로 테스트 메시지를 발행한다
	 * 메시지는 현재 로컬 GameInstance 안에서만 전달된다
	 */
	UFUNCTION(BlueprintCallable, Category = "BARU|Gameplay Message")
	void BroadcastTestMessage(const FString& Text);
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
private:
	void HandleDebugMessage(
		FGameplayTag Channel,
		const FBaruDebugMessage& Payload);
	
	void HandleParentExactMessage(
		FGameplayTag Channel,
		const FBaruDebugMessage& Payload);
	
	void HandleParentPartialMessage(
		FGameplayTag Channel,
		const FBaruDebugMessage& Payload);
	
	FGameplayMessageListenerHandle ChildExactListenerHandle;
	FGameplayMessageListenerHandle ParentExactListenerHandle;
	FGameplayMessageListenerHandle ParentPartialListenerHandle;
	
	int32 NextSequence = 1;
};