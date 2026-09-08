// BaruMainHUDWidget.cpp

#include "UI/HUD/BaruMainHUDWidget.h"

#include "AbilitySystem/Attributes/BaruPlayerAttributeSet.h"
#include "Components/BaruHealthComponent.h"
#include "Player/BaruPlayerState.h"

#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

#include "Engine/World.h"
#include "TimerManager.h"

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
	
	HideInteractionPrompt();
	HideGuideMessage();
	HideWeaponDisplay();
	BindToPlayerState();

	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("Main HUD가 활성화되었습니다. Widget=%s"),
		*GetName());
}

void UBaruMainHUDWidget::NativeOnDeactivated()
{
	HideInteractionPrompt();
	HideGuideMessage();
	HideWeaponDisplay();
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
		BARU_LOG(
			LogBaruUI,
			Warning,
			TEXT("Main HUD에서 BaruPlayerState를 찾지 못했습니다."));
		
		return;
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
}

void UBaruMainHUDWidget::UnbindFromPlayerState()
{
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