// BaruHUD.cpp

#include "UI/BaruHUD.h"

#include "BaruLog.h"
#include "CommonActivatableWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "UI/BaruUITags.h"
#include "UI/Foundation/BaruPrimaryGameLayout.h"
#include "UI/HUD/BaruMainHUDWidget.h"
#include "UI/Subsystem/BaruUIManagerSubsystem.h"

ABaruHUD::ABaruHUD()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABaruHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* OwningPlayerController =
		GetOwningPlayerController();

	/**
	 * Dedicated Server 또는 로컬 플레이어가 아닌 경우
	 * UI를 만들지 않는다
	 */
	if (!IsValid(OwningPlayerController) ||
		!OwningPlayerController->IsLocalController())
	{
		return;
	}

	if (!PrimaryGameLayoutClass)
	{
		BARU_NET_LOG(
			this,
			LogBaruUI,
			Warning,
			TEXT("PrimaryGameLayoutClass가 설정되지 않았습니다."));

		return;
	}

	if (IsValid(PrimaryGameLayout))
	{
		return;
	}

	PrimaryGameLayout =
		CreateWidget<UBaruPrimaryGameLayout>(
			OwningPlayerController,
			PrimaryGameLayoutClass);

	if (!IsValid(PrimaryGameLayout))
	{
		BARU_NET_LOG(
			this,
			LogBaruUI,
			Error,
			TEXT("Primary Game Layout 생성에 실패했습니다."));

		return;
	}

	/**
	 * PrimaryLayout을 먼저 플레이어 화면에 추가한다.
	 * 이 과정에서 BindWidget으로 연결된 Layer Stack들이 준비된다.
	 */
	PrimaryGameLayout->AddToPlayerScreen(0);

	ULocalPlayer* LocalPlayer =
		OwningPlayerController->GetLocalPlayer();

	if (!IsValid(LocalPlayer))
	{
		BARU_NET_LOG(
			this,
			LogBaruUI,
			Error,
			TEXT("LocalPlayer를 찾지 못했습니다."));

		PrimaryGameLayout->RemoveFromParent();
		PrimaryGameLayout = nullptr;

		return;
	}

	UBaruUIManagerSubsystem* UIManager =
		LocalPlayer->GetSubsystem<UBaruUIManagerSubsystem>();

	if (!IsValid(UIManager))
	{
		BARU_NET_LOG(
			this,
			LogBaruUI,
			Error,
			TEXT("UIManagerSubsystem을 찾지 못했습니다."));

		PrimaryGameLayout->RemoveFromParent();
		PrimaryGameLayout = nullptr;

		return;
	}

	/**
	 * UI Manager가 앞으로 사용할 PrimaryLayout을 등록한다.
	 */
	UIManager->RegisterPrimaryLayout(PrimaryGameLayout);

	if (!MainHUDWidgetClass)
	{
		BARU_NET_LOG(
			this,
			LogBaruUI,
			Warning,
			TEXT("MainHUDWidgetClass가 설정되지 않았습니다."));

		return;
	}

	/**
	 * UI.Layer.Game에 Main HUD를 추가한다.
	 */
	UCommonActivatableWidget* MainHUDWidget =
		UIManager->PushWidgetToLayer(
			BaruUITags::UI_Layer_Game.GetTag(),
			MainHUDWidgetClass);

	if (!IsValid(MainHUDWidget))
	{
		BARU_NET_LOG(
			this,
			LogBaruUI,
			Error,
			TEXT("Main HUD를 GameLayer에 추가하지 못했습니다."));

		return;
	}

	BARU_NET_LOG(
		this,
		LogBaruUI,
		Log,
		TEXT("PrimaryGameLayout 등록 및 Main HUD 생성 완료. "
			"Layout=%s, MainHUD=%s"),
			*PrimaryGameLayout->GetName(),
			*MainHUDWidget->GetName());
}

void ABaruHUD::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(PrimaryGameLayout))
	{
		ULocalPlayer* LocalPlayer =
			PrimaryGameLayout->GetOwningLocalPlayer();

		if (IsValid(LocalPlayer))
		{
			UBaruUIManagerSubsystem* UIManager =
				LocalPlayer->GetSubsystem<UBaruUIManagerSubsystem>();

			if (IsValid(UIManager))
			{
				/**
				 * 제거되는 Layout을 UI Manager가 더 이상
				 * 사용하지 않도록 먼저 등록 해제한다.
				 */
				UIManager->UnregisterPrimaryLayout(
					PrimaryGameLayout);
			}
		}

		PrimaryGameLayout->RemoveFromParent();
		PrimaryGameLayout = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}
