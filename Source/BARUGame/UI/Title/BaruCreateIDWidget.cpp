#include "UI/Title/BaruCreateIDWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Subsystems/BaruSaveGameSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "BaruLog.h"

UBaruCreateIDWidget::UBaruCreateIDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputConfig = EBaruWidgetInputMode::Menu;
	GameMouseCaptureMode = EMouseCaptureMode::NoCapture;
	bIsBackHandler = true;
}

void UBaruCreateIDWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (EditBox_IDInput)
	{
		EditBox_IDInput->OnTextChanged.AddUniqueDynamic(this, &UBaruCreateIDWidget::HandleTextChanged);
		EditBox_IDInput->OnTextCommitted.AddUniqueDynamic(this, &UBaruCreateIDWidget::HandleTextCommitted);
	}

	if (Button_Confirm)
	{
		Button_Confirm->OnClicked.AddUniqueDynamic(this, &UBaruCreateIDWidget::HandleConfirmClicked);
		Button_Confirm->SetIsEnabled(false);
	}

	if (Button_Cancel)
	{
		Button_Cancel->OnClicked.AddUniqueDynamic(this, &UBaruCreateIDWidget::HandleCancelClicked);
	}
}

void UBaruCreateIDWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (EditBox_IDInput)
	{
		EditBox_IDInput->SetText(FText::GetEmpty());
		EditBox_IDInput->SetFocus();
	}

	if (Text_ErrorMessage)
	{
		Text_ErrorMessage->SetText(FText::FromString(TEXT("한글/영문 2~12자 이내로 입력해주세요.")));
	}
}

void UBaruCreateIDWidget::HandleTextChanged(const FText& Text)
{
	FText ErrorMsg;
	const bool bIsValid = UBaruSaveGameSubsystem::ValidatePlayerNickname(Text.ToString(), ErrorMsg);

	if (Button_Confirm)
	{
		Button_Confirm->SetIsEnabled(bIsValid);
	}

	if (Text_ErrorMessage)
	{
		Text_ErrorMessage->SetText(bIsValid ? FText::GetEmpty() : ErrorMsg);
	}
}

void UBaruCreateIDWidget::HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		HandleConfirmClicked();
	}
}

void UBaruCreateIDWidget::HandleConfirmClicked()
{
	if (!EditBox_IDInput) return;

	const FString TargetName = EditBox_IDInput->GetText().ToString();
	FText ErrorMsg;

	if (!UBaruSaveGameSubsystem::ValidatePlayerNickname(TargetName, ErrorMsg))
	{
		if (Text_ErrorMessage) Text_ErrorMessage->SetText(ErrorMsg);
		return;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UBaruSaveGameSubsystem* SaveSubsystem = GI->GetSubsystem<UBaruSaveGameSubsystem>())
		{
			if (SaveSubsystem->CreateNewProfile(TargetName))
			{
				OnIDCreationSuccess.Broadcast(TargetName);
				DeactivateWidget();
			}
		}
	}
}

void UBaruCreateIDWidget::HandleCancelClicked()
{
	DeactivateWidget();
}

UWidget* UBaruCreateIDWidget::NativeGetDesiredFocusTarget() const
{
	return EditBox_IDInput;
}