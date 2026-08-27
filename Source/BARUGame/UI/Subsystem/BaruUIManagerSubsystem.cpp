// BaruUIManagerSubsystem.cpp

#include "UI/Subsystem/BaruUIManagerSubsystem.h"

#include "BaruLog.h"
#include "UI/Foundation/BaruPrimaryGameLayout.h"

void UBaruUIManagerSubsystem::RegisterPrimaryLayout(
	UBaruPrimaryGameLayout* InPrimaryLayout)
{
	if (!IsValid(InPrimaryLayout))
	{
		BARU_LOG(
			LogBaruUI,
			Warning,
			TEXT("등록할 PrimaryGameLayout이 유효하지 않습니다."));
		
		return;
	}
	
	if (PrimaryLayout.IsValid() &&
		PrimaryLayout.Get() != InPrimaryLayout)
	{
		BARU_LOG(
			LogBaruUI,
			Warning,
			TEXT("기존 PrimaryGameLayout을 새로운 Layout으로 교체합니다."));
	}
	
	PrimaryLayout = InPrimaryLayout;
	
	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("PrimaryGameLayout 등록 완료. Layout=%s"),
		*InPrimaryLayout->GetName());
}

void UBaruUIManagerSubsystem::UnregisterPrimaryLayout(
	UBaruPrimaryGameLayout* InPrimaryLayout)
{
	if (PrimaryLayout.Get() != InPrimaryLayout)
	{
		return;
	}
	
	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("PrimaryGameLayout 등록 해제. Layout=%s"),
		*GetNameSafe(InPrimaryLayout));
	
	PrimaryLayout.Reset();
}

UCommonActivatableWidget*
	UBaruUIManagerSubsystem::PushWidgetToLayer(
		FGameplayTag LayerTag,
		TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	UBaruPrimaryGameLayout* Layout =
		PrimaryLayout.Get();
	
	if (!IsValid(Layout))
	{
		BARU_LOG(
			LogBaruUI,
			Warning,
			TEXT("등록된 PrimaryGameLayout이 없어 Widget을 열 수 없습니다."));
		
		return nullptr;
	}
	
	return Layout->PushWidgetToLayer(
		LayerTag,
		WidgetClass);
}

bool UBaruUIManagerSubsystem::PopWidgetFromLayer(
	FGameplayTag LayerTag)
{
	UBaruPrimaryGameLayout* Layout =
		PrimaryLayout.Get();
	
	if (!IsValid(Layout))
	{
		BARU_LOG(
			LogBaruUI,
			Warning,
			TEXT("등록된 PrimaryGameLayout이 없어 Widget을 닫을 수 없습니다."));
		
		return false;
	}
	
	return Layout->PopWidgetFromLayer(LayerTag);
}

void UBaruUIManagerSubsystem::Deinitialize()
{
	PrimaryLayout.Reset();
	
	Super::Deinitialize();
}