// BaruSharedAssetWidget.cpp


#include "UI/InventoryUI/BaruSharedAssetWidget.h"

#include "Components/TextBlock.h"
#include "Core/BaruGameState.h"
#include "Engine/World.h"

void UBaruSharedAssetWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindGameState();
}

void UBaruSharedAssetWidget::NativeDestruct()
{
	if (IsValid(BoundGameState))
	{
		BoundGameState->OnTeamScrapValueChanged.RemoveDynamic(
			this,
			&UBaruSharedAssetWidget::
				HandleTeamScrapValueChanged);
	}

	BoundGameState = nullptr;

	Super::NativeDestruct();
}

void UBaruSharedAssetWidget::BindGameState()
{
	if (IsValid(BoundGameState))
	{
		BoundGameState->OnTeamScrapValueChanged.RemoveDynamic(
			this,
			&UBaruSharedAssetWidget::
				HandleTeamScrapValueChanged);
	}

	BoundGameState = GetWorld()
		? GetWorld()->GetGameState<ABaruGameState>()
		: nullptr;

	if (!IsValid(BoundGameState))
	{
		HandleTeamScrapValueChanged(0);
		return;
	}

	BoundGameState->OnTeamScrapValueChanged.AddUniqueDynamic(
		this,
		&UBaruSharedAssetWidget::
			HandleTeamScrapValueChanged);

	// 위젯이 열리기 전에 변경된 값도 즉시 표시
	HandleTeamScrapValueChanged(
		BoundGameState->GetTeamScrapValue());
}

void UBaruSharedAssetWidget::HandleTeamScrapValueChanged(
	int32 NewTeamValue)
{
	if (!IsValid(Text_SharedAsset))
	{
		return;
	}

	Text_SharedAsset->SetText(
		FText::Format(
			FText::FromString(TEXT("공동 자산: {0}")),
			FText::AsNumber(
				FMath::Max(0, NewTeamValue))));
}