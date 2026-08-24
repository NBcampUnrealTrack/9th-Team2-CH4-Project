// BaruGameplayMessageTestComponent.cpp

#include "GameplayMessages/BaruGameplayMessageTestComponent.h"

#include "BaruLog.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayMessages/BaruGameplayMessageTags.h"
#include "GameplayMessages/BaruGameplayMessageTypes.h"

UBaruGameplayMessageTestComponent::UBaruGameplayMessageTestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBaruGameplayMessageTestComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (!UGameplayMessageSubsystem::HasInstance(this))
	{
		BARU_NET_LOG(
			GetOwner(),
			LogBaruUI,
			Warning,
			TEXT("Gameplay Message Subsystem을 찾을 수 없습니다."));
		
		return;
	}
	
	UGameplayMessageSubsystem& MessageSubsystem =
		UGameplayMessageSubsystem::Get(this);
	
	// Message.Debug.Test를 정확히 구독한다.
	ChildExactListenerHandle =
		MessageSubsystem.RegisterListener<FBaruDebugMessage>(
			BaruGameplayMessageTags::Message_Debug_Test,
			this,
			&UBaruGameplayMessageTestComponent::HandleDebugMessage);
	
	// 부모 채널 Message.Debug를 ExactMatch로 구독한다.
	FGameplayMessageListenerParams<FBaruDebugMessage> ParentExactParams;
	ParentExactParams.MatchType = EGameplayMessageMatch::ExactMatch;
	ParentExactParams.SetMessageReceivedCallback(
		this,
		&UBaruGameplayMessageTestComponent::HandleParentExactMessage);
	
	ParentExactListenerHandle =
		MessageSubsystem.RegisterListener<FBaruDebugMessage>(
			BaruGameplayMessageTags::Message_Debug,
			ParentExactParams);
	
	// 부모 채널 Message.Debug를 PartialMatch로 구독한다.
	FGameplayMessageListenerParams<FBaruDebugMessage> ParentPartialParams;
	ParentPartialParams.MatchType = EGameplayMessageMatch::PartialMatch;
	ParentPartialParams.SetMessageReceivedCallback(
		this,
		&UBaruGameplayMessageTestComponent::HandleParentPartialMessage);
	
	ParentPartialListenerHandle =
		MessageSubsystem.RegisterListener<FBaruDebugMessage>(
			BaruGameplayMessageTags::Message_Debug,
			ParentPartialParams);
	
	BARU_NET_LOG(
		GetOwner(),
		LogBaruUI,
		Log,
		TEXT("Message.Debug.Test Listener 3개 등록 완료."));
}

void UBaruGameplayMessageTestComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ChildExactListenerHandle.IsValid())
	{
		ChildExactListenerHandle.Unregister();
	}
	
	if (ParentExactListenerHandle.IsValid())
	{
		ParentExactListenerHandle.Unregister();
	}
	
	if (ParentPartialListenerHandle.IsValid())
	{
		ParentPartialListenerHandle.Unregister();
	}
	
	BARU_NET_LOG(
		GetOwner(),
		LogBaruUI,
		Log,
		TEXT("Gameplay Message Listener 3개 해제 완료"));
	
	Super::EndPlay(EndPlayReason);
}

void UBaruGameplayMessageTestComponent::BroadcastTestMessage(const FString& Text)
{
	if (!UGameplayMessageSubsystem::HasInstance(this))
	{
		BARU_NET_LOG(
			GetOwner(),
			LogBaruUI,
			Warning,
			TEXT("메시지 발행 실패: Gameplay Message Subsystem이 없습니다."));
		
		return;
	}
	
	FBaruDebugMessage Payload;
	Payload.Text = Text;
	Payload.Sequence = NextSequence++;
	
	UGameplayMessageSubsystem& MessageSubsystem = 
		UGameplayMessageSubsystem::Get(this);
	
	MessageSubsystem.BroadcastMessage(
		BaruGameplayMessageTags::Message_Debug_Test,
		Payload);
	
	BARU_NET_LOG(
		GetOwner(),
		LogBaruUI,
		Log,
		TEXT("테스트 메시지 발행: Sequence=%d, Text=%s"),
		Payload.Sequence,
		*Payload.Text);
}

void UBaruGameplayMessageTestComponent::HandleDebugMessage(
	FGameplayTag Channel,
	const FBaruDebugMessage& Payload)
{
	BARU_NET_LOG(
		GetOwner(),
		LogBaruUI,
		Log,
		TEXT("테스트 메시지 수신: Channel=%s, Sequence=%d, Text=%s"),
		*Channel.ToString(),
			Payload.Sequence,
			*Payload.Text);
}

void UBaruGameplayMessageTestComponent::HandleParentExactMessage(
	FGameplayTag Channel,
	const FBaruDebugMessage& Payload)
{
	BARU_NET_LOG(
		GetOwner(),
		LogBaruUI,
		Log,
		TEXT("부모 Exact Listener 수신: Channel=%s, Sequence=%d"),
		*Channel.ToString(),
		Payload.Sequence);
}

void UBaruGameplayMessageTestComponent::HandleParentPartialMessage(
	FGameplayTag Channel,
	const FBaruDebugMessage& Payload)
{
	BARU_NET_LOG(
		GetOwner(),
		LogBaruUI,
		Log,
		TEXT("부모 Partial Listener 수신: Channel=%s, Sequence=%d"),
		*Channel.ToString(),
		Payload.Sequence);
}