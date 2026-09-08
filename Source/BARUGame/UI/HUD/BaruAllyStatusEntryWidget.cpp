// BaruAllyStatusEntryWidget.cpp

#include "UI/HUD/BaruAllyStatusEntryWidget.h"

#include "AbilitySystem/Attributes/BaruPlayerAttributeSet.h"
#include "Components/BaruHealthComponent.h"
#include "Player/BaruPlayerState.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UBaruAllyStatusEntryWidget::NativeOnListItemObjectSet(
	UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(
		ListItemObject);
	
	/**
	 *	ListView가 이 위젯을 재사용할 수 있으므로
	 *	이전 플레이어와 연결된 이벤트를 먼저 해제한다.
	 */
	UnbindFromPlayerState();
	
	BoundPlayerState =
		Cast<ABaruPlayerState>(ListItemObject);
	
	if (!IsValid(BoundPlayerState))
	{
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
	
	BoundPlayerState->OnDBNOStatusChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleDBNOStatusChanged);
	
	BoundPlayerState->OnDeadStatusChanged.AddUniqueDynamic(
		this,
		&ThisClass::HandleDeadStatusChanged);
	
	RefreshDisplay();
}

void UBaruAllyStatusEntryWidget::NativeDestruct()
{
	UnbindFromPlayerState();
	
	Super::NativeDestruct();
}

void UBaruAllyStatusEntryWidget::UnbindFromPlayerState()
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
		
		BoundPlayerState->OnDBNOStatusChanged.RemoveDynamic(
			this,
			&ThisClass::HandleDBNOStatusChanged);
		
		BoundPlayerState->OnDeadStatusChanged.RemoveDynamic(
			this,
			&ThisClass::HandleDeadStatusChanged);
	}
	
	BoundHealthComponent = nullptr;
	BoundPlayerState = nullptr;
}

void UBaruAllyStatusEntryWidget::RefreshDisplay()
{
	if (!IsValid(BoundPlayerState))
	{
		return;
	}
	
	if (IsValid(Text_AllyName))
	{
		FString PlayerName =
			BoundPlayerState->GetPlayerName();
		
		if (PlayerName.IsEmpty())
		{
			PlayerName = TEXT("플레이어");
		}
		
		Text_AllyName->SetText(
			FText::FromString(PlayerName));
	}
	
	if (IsValid(Text_AllyState))
	{
		FText StateText;
		
		if (BoundPlayerState->IsDead())
		{
			StateText = FText::FromString(TEXT("사망"));
		}
		else if (BoundPlayerState->IsDBNO())
		{
			StateText = FText::FromString(TEXT("행동 불능"));
		}
		else
		{
			StateText = FText::FromString(TEXT("생존"));
		}
		
		Text_AllyState->SetText(StateText);
	}
	
	const float Health = BoundPlayerState->GetHealth();
	
	const float MaxHealth = BoundPlayerState->GetMaxHealth();
	
	if (IsValid(ProgressBar_AllyHealth))
	{
		const float HealthPercent =
			MaxHealth > 0.0f 
			? Health / MaxHealth
			: 0.0f;
		
		ProgressBar_AllyHealth->SetPercent(FMath::Clamp(HealthPercent, 0.0f, 1.0f));
	}
	
	const UBaruPlayerAttributeSet* AttributeSet =
		BoundPlayerState->GetPlayerAttributeSet();
	
	if (IsValid(ProgressBar_AllySanity) && 
		IsValid(AttributeSet))
	{
		const float Sanity =
			AttributeSet->GetSanity();
		
		const float MaxSanity =
			AttributeSet->GetMaxSanity();
		
		const float SanityPercent =
			MaxSanity > 0.0f
			? Sanity / MaxSanity
			: 0.0f;
		
		ProgressBar_AllySanity->SetPercent(
			FMath::Clamp(SanityPercent, 0.0f, 1.0f));
	}
}

void UBaruAllyStatusEntryWidget::HandleHealthChanged(
	UBaruHealthComponent* HealthComponent,
	float OldHealth,
	float NewHealth,
	AActor* Instigator)
{
	RefreshDisplay();
}

void UBaruAllyStatusEntryWidget::HandleMaxHealthChanged(
	float OldMaxHealth,
	float NewMaxHealth)
{
	RefreshDisplay();
}

void UBaruAllyStatusEntryWidget::HandleSanityChanged(
	float NewSanity)
{
	RefreshDisplay();
}

void UBaruAllyStatusEntryWidget::HandleDBNOStatusChanged(
	bool bIsDBNO)
{
	RefreshDisplay();
}

void UBaruAllyStatusEntryWidget::HandleDeadStatusChanged(
	bool bIsDead)
{
	RefreshDisplay();
}