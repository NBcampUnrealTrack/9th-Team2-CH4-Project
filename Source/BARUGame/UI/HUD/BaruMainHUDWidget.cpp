// BaruMainHUDWidget.cpp

#include "UI/HUD/BaruMainHUDWidget.h"
#include "Components/BaruHealthComponent.h"
#include "AbilitySystem/Attributes/BaruPlayerAttributeSet.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

#include "BaruLog.h"
#include "Player/BaruPlayerState.h"

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

void UBaruMainHUDWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	
	BindToPlayerState();

	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("Main HUD가 활성화되었습니다. Widget=%s"),
		*GetName());
}

void UBaruMainHUDWidget::NativeOnDeactivated()
{
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
					TEXT("%0.f / %0.f"),
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