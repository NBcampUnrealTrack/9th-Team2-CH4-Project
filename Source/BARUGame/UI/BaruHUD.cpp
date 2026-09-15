// BaruHUD.cpp

#include "UI/BaruHUD.h"

#include "BaruLog.h"
#include "CommonActivatableWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "UI/Foundation/BaruPrimaryGameLayout.h"
#include "UI/Subsystem/BaruUIManagerSubsystem.h"
#include "UI/BaruUITags.h"
#include "UI/Result/BaruSettlementResultWidget.h"

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
	
	BoundPlayerController =
		Cast<ABaruPlayerController>(OwningPlayerController);
	
	if (IsValid(BoundPlayerController))
	{
		BoundPlayerController->OnSettlementReceived.AddUniqueDynamic(
			this,
			&ThisClass::HandleSettlementReceived);
	}
	else
	{
		BARU_NET_LOG(
			this,
			LogBaruUI,
			Warning,
			TEXT("정산 결과를 연결할 BaruPlayerController를 찾지 못했습니다."));
	}

	if (!InitialWidgetClass)
	{
		BARU_NET_LOG(
			this,
			LogBaruUI,
			Warning,
			TEXT("InitialWidgetClass가 설정되지 않았습니다."));

		return;
	}
	
	if (!InitialWidgetLayerTag.IsValid())
	{
		BARU_NET_LOG(
			this,
			LogBaruUI,
			Warning,
			TEXT("InitialWidgetLayerTag가 설정되지 않았습니다.")
			);
		
		return;
	}

	/**
	 * 설정된 초기 레이어에 초기 위젯을 추가한다.
	 *
	 * 예:
	 * - 게임 맵: UI.Layer.Game + WBP_MainHUD
	 * - 메뉴 맵: UI.Layer.Menu + WBP_Title
	 */
	UCommonActivatableWidget* InitialWidget =
		UIManager->PushWidgetToLayer(
			InitialWidgetLayerTag,
			InitialWidgetClass);

	if (!IsValid(InitialWidget))
	{
		BARU_NET_LOG(
			this,
			LogBaruUI,
			Error,
			TEXT("초기 Widget을 Layer에 추가하지 못했습니다. "
				"LayerTag=%s"
			),
			*InitialWidgetLayerTag.ToString()
			);

		return;
	}

	InitialWidget->SetVisibility(
		ESlateVisibility::SelfHitTestInvisible);

	BARU_NET_LOG(
		this,
		LogBaruUI,
		Log,
		TEXT("PrimaryGameLayout 등록 및 초기 Widget 생성 완료. "
			"Layout=%s, LayerTag=%s, Widget=%s"),
			*PrimaryGameLayout->GetName(),
			*InitialWidgetLayerTag.ToString(),
			*InitialWidget->GetName());
}

void ABaruHUD::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(BoundPlayerController))
	{
		BoundPlayerController->OnSettlementReceived.RemoveDynamic(
			this,
			&ThisClass::HandleSettlementReceived);
		
		BoundPlayerController = nullptr;
	}
	
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

void ABaruHUD::HandleSettlementReceived(
	const FBaruSettlementReport& Report)
{
	if (!SettlementResultWidgetClass)
	{
		BARU_NET_LOG(
			this,
			LogBaruUI,
			Warning,
			TEXT("SettlementResultWidgetClass가 설정되지 않았습니다."));
		
		return;
	}
	
	if (!IsValid(BoundPlayerController))
	{
		return;
	}
	
	ULocalPlayer* LocalPlayer =
		BoundPlayerController->GetLocalPlayer();
	
	if (!IsValid(LocalPlayer))
	{
		return;
	}
	
	UBaruUIManagerSubsystem* UIManager =
		LocalPlayer->GetSubsystem<UBaruUIManagerSubsystem>();
	
	if (!IsValid(UIManager))
	{
		return;
	}
	
	UCommonActivatableWidget* AddedWidget =
		UIManager->PushWidgetToLayer(
			BaruUITags::UI_Layer_Modal.GetTag(),
			SettlementResultWidgetClass);
	
	UBaruSettlementResultWidget* ResultWidget =
		Cast<UBaruSettlementResultWidget>(AddedWidget);
	
	if (!IsValid(ResultWidget))
	{
		BARU_NET_LOG(
			this,
			LogBaruUI,
			Error,
			TEXT("정산 결과가 Widget 생성에 실패했습니다."));
		
		return;
	}
	
	ResultWidget->SetResultData(
		Report.bSurvived,
		Report.MonsterKillCount,
		Report.ExtractedItemCount,
		Report.AcquiredCurrency);
	
	BARU_NET_LOG(
		this,
		LogBaruUI,
		Log,
		TEXT("정산 결과 Widget을 Modal Layer에 표시했습니다. (Kills: %d, Items: %d, Currency: %d)"),
		Report.MonsterKillCount,
		Report.ExtractedItemCount,
		Report.AcquiredCurrency);
}