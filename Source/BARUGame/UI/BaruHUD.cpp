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

	APlayerController* OwningPlayerController = GetOwningPlayerController();
	if (!IsValid(OwningPlayerController) || !OwningPlayerController->IsLocalController())
	{
		return;
	}

	if (!PrimaryGameLayoutClass)
	{
		BARU_NET_LOG(this, LogBaruUI, Warning, TEXT("PrimaryGameLayoutClass가 설정되지 않았습니다."));
		return;
	}

	if (IsValid(PrimaryGameLayout))
	{
		return;
	}

	PrimaryGameLayout = CreateWidget<UBaruPrimaryGameLayout>(OwningPlayerController, PrimaryGameLayoutClass);
	if (!IsValid(PrimaryGameLayout))
	{
		BARU_NET_LOG(this, LogBaruUI, Error, TEXT("Primary Game Layout 생성에 실패했습니다."));
		return;
	}

	PrimaryGameLayout->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	PrimaryGameLayout->AddToPlayerScreen(0);

	ULocalPlayer* LocalPlayer = OwningPlayerController->GetLocalPlayer();
	if (!IsValid(LocalPlayer))
	{
		return;
	}

	UBaruUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UBaruUIManagerSubsystem>();
	if (!IsValid(UIManager))
	{
		return;
	}

	UIManager->RegisterPrimaryLayout(PrimaryGameLayout);

	// 단 1회만 Main HUD 생성 및 등록
	if (MainHUDWidgetClass)
	{
		UCommonActivatableWidget* MainHUDWidget = UIManager->PushWidgetToLayer(BaruUITags::UI_Layer_Game.GetTag(), MainHUDWidgetClass);
		if (IsValid(MainHUDWidget))
		{
			MainHUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			BARU_NET_LOG(this, LogBaruUI, Log, TEXT("PrimaryGameLayout 등록 및 Main HUD 생성 완료. Layout=%s, MainHUD=%s"),
				*PrimaryGameLayout->GetName(), *MainHUDWidget->GetName());
		}
	}
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
