// BaruMainHUDWidget.cpp

#include "UI/HUD/BaruMainHUDWidget.h"

#include "AbilitySystem/Attributes/BaruPlayerAttributeSet.h"
#include "Components/BaruHealthComponent.h"
#include "Player/BaruPlayerState.h"
#include "Core/BaruGameState.h"

#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/ListView.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/PlayerState.h"
#include "Animation/WidgetAnimation.h"
#include "Gameplay/Inventory/BaruInventoryComponent.h"

#include "BaruLog.h"


UBaruMainHUDWidget::UBaruMainHUDWidget(
	const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
{
	// [08.30] CommonUI 포커스로 인한 자동 비활성화 차단
	bSupportsActivationFocus = false;
	bIsBackHandler = false;
	bAutoActivate = true;
	SetIsFocusable(false);
	
	// Main HUD는 플레이 중 항상 표시되지만
	// 캐릭터와 카메라 입력을 막으면 안 된다.
	InputConfig = EBaruWidgetInputMode::Game;

	GameMouseCaptureMode =
		EMouseCaptureMode::CapturePermanently;
}

void UBaruMainHUDWidget::ShowInteractionPrompt(
	const FText& PromptText)
{
	if (PromptText.IsEmpty())
	{
		HideInteractionPrompt();
		return;
	}
	
	if (IsValid(Text_InteractionPrompt))
	{
		Text_InteractionPrompt->SetText(PromptText);
	}
	
	if (IsValid(Border_InteractionPrompt))
	{
		Border_InteractionPrompt->SetVisibility(
			ESlateVisibility::HitTestInvisible);
	}
}

void UBaruMainHUDWidget::HideInteractionPrompt()
{
	if (IsValid(Border_InteractionPrompt))
	{
		Border_InteractionPrompt->SetVisibility(
			ESlateVisibility::Collapsed);
	}
}

void UBaruMainHUDWidget::ShowGuideMessage(
	const FText& Message,
	float Duration)
{
	if (Message.IsEmpty())
	{
		HideGuideMessage();
		return;
	}
	
	if (IsValid(Text_GuideMessage))
	{
		Text_GuideMessage->SetText(Message);
	}
	
	if (IsValid(Border_GuideMessage))
	{
		Border_GuideMessage->SetVisibility(
			ESlateVisibility::HitTestInvisible);
	}
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(
			GuideMessageTimerHandle);
		
		if (Duration > 0.0f)
		{
			World->GetTimerManager().SetTimer(
				GuideMessageTimerHandle,
				this,
				&ThisClass::HideGuideMessage,
				Duration,
				false);
		}
	}
}

void UBaruMainHUDWidget::HideGuideMessage()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(
			GuideMessageTimerHandle);
	}
	
	if (IsValid(Border_GuideMessage))
	{
		Border_GuideMessage->SetVisibility(
			ESlateVisibility::Collapsed);
	}
}

void UBaruMainHUDWidget::ShowWeaponDisplay(
	const FText& WeaponName,
	int32 CurrentAmmo,
	int32 ReserveAmmo)
{
	if (WeaponName.IsEmpty())
	{
		HideWeaponDisplay();
		return;
	}
	
	if (IsValid(Text_WeaponName))
	{
		Text_WeaponName->SetText(WeaponName);
	}
	
	if (IsValid(Text_CurrentAmmo))
	{
		Text_CurrentAmmo->SetText(
			FText::AsNumber(FMath::Max(0, CurrentAmmo)));
	}
	
	if (IsValid(Text_ReserveAmmo))
	{
		Text_ReserveAmmo->SetText(
			FText::AsNumber(FMath::Max(0, ReserveAmmo)));
	}
	
	if (IsValid(Border_WeaponStatus))
	{
		Border_WeaponStatus->SetVisibility(
			ESlateVisibility::HitTestInvisible);
	}
}

void UBaruMainHUDWidget::HideWeaponDisplay()
{
	if (IsValid(Border_WeaponStatus))
	{
		Border_WeaponStatus->SetVisibility(
			ESlateVisibility::Collapsed);
	}
}

void UBaruMainHUDWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	
	// 기존 HUD 초기화
	HideInteractionPrompt();
	HideGuideMessage();
	HideWeaponDisplay();
	
	// 아이템 획득 알림을 처음에는 숨긴다.
	HidePickupNotification();
	
	// 부활 진행 UI를 처음에는 숨긴다.
	HideReviveProgress();
	
	// 기존 피격 효과 초기화
	if (IsValid(Border_HitScreenEffect))
	{
		Border_HitScreenEffect->SetRenderOpacity(0.0f);
	}
	
	PlayerStateBindRetryCount = 0;
	
	// 기존 PlayerState 연결
	// 내부에서 InventoryComponent도 연결할 예정
	BindToPlayerState();
	
	// 기존 GameState 연결
	BindToGameState();
	
	// 부활 시작/종료 이벤트를 받기 위해
	// PlayerController와 연결한다.
	BindToPlayerController();
	
	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("Main HUD가 활성화되었습니다. Widget=%s"),
		*GetName());
}

void UBaruMainHUDWidget::NativeOnDeactivated()
{
	// 기존 HUD 정리
	HideInteractionPrompt();
	HideGuideMessage();
	HideWeaponDisplay();
	
	// 아이템 알림 애니메이션 및 UI 정리
	HidePickupNotification();
	
	// 부활 진행 애니메이션 및 UI 정리
	HideReviveProgress();
	
	// 기존 피격 애니메이션 정리
	if (IsValid(Anim_HitScreenEffect))
	{
		StopAnimation(Anim_HitScreenEffect);
	}
	
	if (IsValid(Border_HitScreenEffect))
	{
		Border_HitScreenEffect->SetRenderOpacity(0.0f);
	}
	
	// PlayerController 부활 이벤트 해제
	UnbindFromPlayerController();
	
	// InventoryComponent 획득 이벤트 해제
	UnbindFromInventoryComponent();
	
	// 기존 연결 해제
	UnbindFromGameState();
	UnbindFromPlayerState();
	
	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("Main HUD가 비활성화되었습니다. Widget=%s"),
		*GetName());
	
	Super::NativeOnDeactivated();
}

void UBaruMainHUDWidget::BindToPlayerState()
{
	UnbindFromPlayerState();
	
	BoundPlayerState =
		GetOwningPlayerState<ABaruPlayerState>();
	
	if (!IsValid(BoundPlayerState))
	{
		// 클라이언트에서는 PlayerState 복제가 HUD 생성보다
		// 조금 늦을 수 있으므로 최대 20회 재시도한다.
		if (PlayerStateBindRetryCount < 20)
		{
			++PlayerStateBindRetryCount;
			
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(
					PlayerStateBindRetryTimerHandle,
					this,
					&ThisClass::BindToPlayerState,
					0.1f,
					false);
			}
		}
		else
		{
			BARU_LOG(
				LogBaruUI,
				Warning,
				TEXT("Main HUD에서 BaruPlayerState 연결에 실패했습니다."));
		}
		
		return;
	}
	
	PlayerStateBindRetryCount = 0;
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(
			PlayerStateBindRetryTimerHandle);
	}
	
	BoundHealthComponent =
		BoundPlayerState->GetHealthComponent();
	
	if (IsValid(BoundHealthComponent))
	{
		BoundHealthComponent->OnHealthChanged.AddUniqueDynamic(
			this,
			&ThisClass::HandleHealthChanged);
		
		BoundHealthComponent->OnMaxHealthChanged.AddUniqueDynamic(
			this,
			&ThisClass::HandleMaxHealthChanged);
		
	}
	
	BoundPlayerState->OnSanityChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleSanityChanged);
	
	RefreshPlayerStatus();
	
	// 클라이언트 PlayerState가 준비된 후 아군 목록도 다시 갱신
	RebuildAllyStatusList();
	
	BindToInventoryComponent();
}

void UBaruMainHUDWidget::UnbindFromPlayerState()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(
			PlayerStateBindRetryTimerHandle);
	}
	
	if (IsValid(BoundHealthComponent))
	{
		BoundHealthComponent->OnHealthChanged.RemoveDynamic(
			this,
			&ThisClass::HandleHealthChanged);
		
		BoundHealthComponent->OnMaxHealthChanged.RemoveDynamic(
			this,
			&ThisClass::HandleMaxHealthChanged);
	}
	
	if (IsValid(BoundPlayerState))
	{
		BoundPlayerState->OnSanityChanged.RemoveDynamic(
			this,
			&ThisClass::HandleSanityChanged);
	}
	
	BoundHealthComponent = nullptr;
	BoundPlayerState = nullptr;
}

void UBaruMainHUDWidget::BindToGameState()
{
	UnbindFromGameState();
	
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}
	
	BoundGameState =
		World->GetGameState<ABaruGameState>();
	
	if (!IsValid(BoundGameState))
	{
		BARU_LOG(
			LogBaruUI,
			Warning,
			TEXT("Main HUD에서 BaruGameState를 찾지 못했습니다."));
		
		return;
	}
	
	BoundGameState->OnAlivePlayerCountChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleAlivePlayerCountChanged);
	
	// 서버에서 전달된 전역 알림을 안내 메시지로 표시한다.
	BoundGameState->OnGlobalNotificationReceived.AddUniqueDynamic(
		this,
		&ThisClass::ShowGuideMessage);
	
	RebuildAllyStatusList();
}

void UBaruMainHUDWidget::UnbindFromGameState()
{
	if (IsValid(BoundGameState))
	{
		BoundGameState->OnAlivePlayerCountChanged.RemoveDynamic(
			this,
			&ThisClass::HandleAlivePlayerCountChanged);
		
		BoundGameState->OnGlobalNotificationReceived.RemoveDynamic(
			this,
			&ThisClass::ShowGuideMessage);
	}
	
	if (IsValid(ListView_AllyStatus))
	{
		ListView_AllyStatus->ClearListItems();
	}
	
	BoundGameState = nullptr;
}

void UBaruMainHUDWidget::RebuildAllyStatusList()
{
	if (!IsValid(ListView_AllyStatus) ||
		!IsValid(BoundGameState))
	{
		return;
	}
	
	ListView_AllyStatus->ClearListItems();
	
	for (APlayerState* PlayerState :
		BoundGameState->PlayerArray)
	{
		ABaruPlayerState* AllyPlayerState =
			Cast<ABaruPlayerState>(PlayerState);
		
		if (!IsValid(AllyPlayerState))
		{
			continue;
		}
		
		// 자신의 체력, 정신력은 기존 개인 상태 UI에 표시되므로 제외한다.
		if (AllyPlayerState == BoundPlayerState)
		{
			continue;
		}
		
		ListView_AllyStatus->AddItem(
			AllyPlayerState);
	}
}

void UBaruMainHUDWidget::HandleAlivePlayerCountChanged(
	int32 NewAliveCount)
{
	RebuildAllyStatusList();
}

void UBaruMainHUDWidget::RefreshPlayerStatus()
{
	UpdateHealthDisplay();
	UpdateSanityDisplay();
}

void UBaruMainHUDWidget::UpdateHealthDisplay()
{
	if (!IsValid(BoundHealthComponent))
	{
		return;
	}
	
	const float Health = 
		BoundHealthComponent->GetHealth();
	
	const float MaxHealth =
		BoundHealthComponent->GetMaxHealth();
	
	if (IsValid(ProgressBar_Health))
	{
		ProgressBar_Health->SetPercent(
			BoundHealthComponent->GetHealthNormalized());
	}
	
	if (IsValid(Text_HealthValue))
	{
		Text_HealthValue->SetText(
			FText::FromString(
				FString::Printf(
					TEXT("%.0f / %.0f"),
					Health,
					MaxHealth)));
	}
}

void UBaruMainHUDWidget::UpdateSanityDisplay()
{
	if (!IsValid(BoundPlayerState))
	{
		return;
	}
	
	const UBaruPlayerAttributeSet* AttributeSet =
		BoundPlayerState->GetPlayerAttributeSet();
	
	if (!IsValid(AttributeSet))
	{
		return;
	}
	
	const float Sanity = AttributeSet->GetSanity();
	const float MaxSanity = AttributeSet->GetMaxSanity();
	
	if (IsValid(ProgressBar_Sanity))
	{
		const float SanityPercent = 
			MaxSanity > 0.0f
			? Sanity / MaxSanity
			: 0.0f;
		
		ProgressBar_Sanity->SetPercent(
			FMath::Clamp(SanityPercent, 0.0f, 1.0f));
	}
	
	if (IsValid(Text_SanityValue))
	{
		Text_SanityValue->SetText(
			FText::FromString(
				FString::Printf(
					TEXT("%.0f / %.0f"),
					Sanity,
					MaxSanity)));
	}
}

void UBaruMainHUDWidget::HandleHealthChanged(
	UBaruHealthComponent* HealthComponent,
	float OldHealth,
	float NewHealth,
	AActor* Instigator)
{
	UpdateHealthDisplay();

	// 초기화나 회복이 아니라 실제로 체력이 감소했을 때만 재생한다.
	if (NewHealth < OldHealth - KINDA_SMALL_NUMBER)
	{
		PlayHitScreenEffect();
	}
}

void UBaruMainHUDWidget::HandleMaxHealthChanged(
	float OldMaxHealth,
	float NewMaxHealth)
{
	UpdateHealthDisplay();
}

void UBaruMainHUDWidget::HandleSanityChanged(float NewSanity)
{
	UpdateSanityDisplay();
}

void UBaruMainHUDWidget::PlayHitScreenEffect()
{
	if (!IsValid(Border_HitScreenEffect) ||
		!IsValid(Anim_HitScreenEffect))
	{
		return;
	}

	// 연속으로 피격될 경우 이전 재생을 중단하고 처음부터 다시 재생한다.
	StopAnimation(Anim_HitScreenEffect);

	PlayAnimation(
		Anim_HitScreenEffect,
		0.0f,
		1,
		EUMGSequencePlayMode::Forward,
		1.0f);
}

void UBaruMainHUDWidget::BindToInventoryComponent()
{
	UnbindFromInventoryComponent();
	
	if (!IsValid(BoundPlayerState))
	{
		return;
	}
	
	BoundInventoryComponent =
		BoundPlayerState->GetInventoryComponent();
	
	if (IsValid(BoundInventoryComponent))
	{
		BoundInventoryComponent->OnInventoryPickupResult.AddUniqueDynamic(
			this,
			&ThisClass::HandleInventoryPickupResult);
	}
}

void UBaruMainHUDWidget::UnbindFromInventoryComponent()
{
	if (IsValid(BoundInventoryComponent))
	{
		BoundInventoryComponent->OnInventoryPickupResult.RemoveDynamic(
			this,
			&ThisClass::HandleInventoryPickupResult);
	}
	
	BoundInventoryComponent = nullptr;
}

void UBaruMainHUDWidget::HandleInventoryPickupResult(
	const FBaruInventoryPickupNotification& Notification)
{
	if (!IsValid(Border_PickupNotification) ||
		!IsValid(Text_PickupTitle) ||
		!IsValid(Text_PickupDetail))
	{
		return;
	}
	
	switch (Notification.Result)
	{
	case EBaruInventoryPickupResult::Succeeded:
		Text_PickupTitle->SetText(
			FText::FromString(TEXT("아이템 획득")));
		
		Text_PickupDetail->SetText(
			FText::Format(
				NSLOCTEXT(
					"BaruHUD",
					"PickupSucceeded",
					"{0} × {1}"),
				Notification.ItemName,
				FText::AsNumber(Notification.AddedQuantity)));
		break;
		
	case EBaruInventoryPickupResult::Partial:
		Text_PickupTitle->SetText(
			FText::FromString(TEXT("일부만 획득")));

		Text_PickupDetail->SetText(
			FText::Format(
				NSLOCTEXT(
					"BaruHUD",
					"PickupPartial",
					"{0} × {1} 획득 · {2}개 남음"),
				Notification.ItemName,
				FText::AsNumber(Notification.AddedQuantity),
				FText::AsNumber(Notification.RemainingQuantity)));
		break;

	case EBaruInventoryPickupResult::Full:
		Text_PickupTitle->SetText(
			FText::FromString(TEXT("인벤토리 공간 부족")));

		Text_PickupDetail->SetText(Notification.ItemName);
		break;
	}
	
	Border_PickupNotification->SetVisibility(
		ESlateVisibility::HitTestInvisible);
	
	if (IsValid(Anim_PickupNotification))
	{
		StopAnimation(Anim_PickupNotification);
		PlayAnimation(Anim_PickupNotification);
	}
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(
			PickupNotificationTimerHandle);
		
		World->GetTimerManager().SetTimer(
			PickupNotificationTimerHandle,
			this,
			&ThisClass::HidePickupNotification,
			4.05f,
			false);
	}
}

void UBaruMainHUDWidget::HidePickupNotification()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(
			PickupNotificationTimerHandle);
	}
	
	if (IsValid(Border_PickupNotification))
	{
		Border_PickupNotification->SetVisibility(
			ESlateVisibility::Collapsed);
		
		Border_PickupNotification->SetRenderOpacity(0.0f);
	}
}

void UBaruMainHUDWidget::BindToPlayerController()
{
	UnbindFromPlayerController();
	
	BoundPlayerController =
		Cast<ABaruPlayerController>(GetOwningPlayer());
	
	if (!IsValid(BoundPlayerController))
	{
		return;
	}
	
	BoundPlayerController->OnInteractionHoldStarted.AddUniqueDynamic(
		this,
		&ThisClass::HandleInteractionHoldStarted);
	
	BoundPlayerController->OnInteractionHoldEnded.AddUniqueDynamic(
		this,
		&ThisClass::HandleInteractionHoldEnded);
}

void UBaruMainHUDWidget::UnbindFromPlayerController()
{
	if (IsValid(BoundPlayerController))
	{
		BoundPlayerController->OnInteractionHoldStarted.RemoveDynamic(
			this,
			&ThisClass::HandleInteractionHoldStarted);
		
		BoundPlayerController->OnInteractionHoldEnded.RemoveDynamic(
			this,
			&ThisClass::HandleInteractionHoldEnded);
	}
	
	BoundPlayerController = nullptr;
}

void UBaruMainHUDWidget::HandleInteractionHoldStarted(
	AActor* OtherActor,
	float Duration,
	bool bIsHolder)
{
	if (!IsValid(Border_ReviveProgress) ||
		!IsValid(ProgressBar_ReviveProgressBar))
	{
		return;
	}
	
	Border_ReviveProgress->SetVisibility(
		ESlateVisibility::HitTestInvisible);
	
	ProgressBar_ReviveProgressBar->SetPercent(0.0f);
	
	if (IsValid(Text_ReviveProgress))
	{
		Text_ReviveProgress->SetText(
			FText::FromString(
				bIsHolder
				? TEXT("팀원 치료 중...")
				: TEXT("치료 받는 중...")));
	}
	
	if (IsValid(Anim_ReviveProgress))
	{
		StopAnimation(Anim_ReviveProgress);
		
		// Anim_ReviveProgress가 1초이므로
		// 실제 Duration초에 맞춰 재생 속도를 조절한다.
		const float PlaybackSpeed =
			1.0f / FMath::Max(Duration, 0.01f);
		
		PlayAnimation(
			Anim_ReviveProgress,
			0.0f,
			1,
			EUMGSequencePlayMode::Forward,
			PlaybackSpeed);
	}
}

void UBaruMainHUDWidget::HandleInteractionHoldEnded(
	AActor* OtherActor,
	EBaruInteractionHoldEndReason Reason,
	bool bIsHolder)
{
	HideReviveProgress();
}

void UBaruMainHUDWidget::HideReviveProgress()
{
	if (IsValid(Anim_ReviveProgress))
	{
		StopAnimation(Anim_ReviveProgress);
	}
	
	if (IsValid(ProgressBar_ReviveProgressBar))
	{
		ProgressBar_ReviveProgressBar->SetPercent(0.0f);
	}
	
	if (IsValid(Border_ReviveProgress))
	{
		Border_ReviveProgress->SetVisibility(
			ESlateVisibility::Collapsed);
	}
}
