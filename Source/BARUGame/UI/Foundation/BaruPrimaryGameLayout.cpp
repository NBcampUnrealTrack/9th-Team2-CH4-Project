// BaruPrimaryGameLayout.cpp

#include "UI/Foundation/BaruPrimaryGameLayout.h"

#include "BaruLog.h"
#include "CommonActivatableWidget.h"
#include "UI/BaruUITags.h"

UCommonActivatableWidget*
	UBaruPrimaryGameLayout::PushWidgetToLayer(
		FGameplayTag LayerTag,
		TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	if (!WidgetClass)
	{
		BARU_LOG(
			LogBaruUI,
			Warning,
			TEXT("추가할 WidgetClass가 설정되지 않았습니다."));

		return nullptr;
	}

	UCommonActivatableWidgetStack* LayerStack =
		GetLayerStack(LayerTag);

	if (!IsValid(LayerStack))
	{
		BARU_LOG(
			LogBaruUI,
			Warning,
			TEXT("UI Layer를 찾지 못했습니다. LayerTag=%s"),
			*LayerTag.ToString());

		return nullptr;
	}

	UCommonActivatableWidget* AddedWidget =
		LayerStack->AddWidget<UCommonActivatableWidget>(WidgetClass);

	if (!IsValid(AddedWidget))
	{
		BARU_LOG(
			LogBaruUI,
			Error,
			TEXT("Widget을 Layer에 추가하지 못했습니다. LayerTag=%s"),
			*LayerTag.ToString());

		return nullptr;
	}

	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("Widget을 Layer에 추가했습니다. LayerTag=%s, Widget=%s"),
		*LayerTag.ToString(),
		*AddedWidget->GetName());

	return AddedWidget;
}

bool UBaruPrimaryGameLayout::PopWidgetFromLayer(
	FGameplayTag LayerTag)
{
	UCommonActivatableWidgetStack* LayerStack =
		GetLayerStack(LayerTag);

	if (!IsValid(LayerStack))
	{
		BARU_LOG(
			LogBaruUI,
			Warning,
			TEXT("UI Layer를 찾지 못했습니다. LayerTag=%s"),
			*LayerTag.ToString());

		return false;
	}

	UCommonActivatableWidget* ActiveWidget =
		LayerStack->GetActiveWidget();

	if (!IsValid(ActiveWidget))
	{
		BARU_LOG(
			LogBaruUI,
			Warning,
			TEXT("닫을 활성 Widget이 없습니다. LayerTag=%s"),
			*LayerTag.ToString());

		return false;
	}

	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("활성 Widget을 닫습니다. LayerTag=%s, Widget=%s"),
		*LayerTag.ToString(),
		*ActiveWidget->GetName());

	ActiveWidget->DeactivateWidget();

	return true;
}

UCommonActivatableWidgetStack*
	UBaruPrimaryGameLayout::GetLayerStack(
		FGameplayTag LayerTag) const
{
	if (LayerTag.MatchesTagExact(
		BaruUITags::UI_Layer_Game.GetTag()))
	{
		return GameLayer;
	}

	if (LayerTag.MatchesTagExact(
		BaruUITags::UI_Layer_GameMenu.GetTag()))
	{
		return GameMenuLayer;
	}
	
	if (LayerTag.MatchesTagExact(
		BaruUITags::UI_Layer_Menu.GetTag()))
	{
		return MenuLayer;
	}

	if (LayerTag.MatchesTagExact(
		BaruUITags::UI_Layer_Modal.GetTag()))
	{
		return ModalLayer;
	}

	return nullptr;
}
