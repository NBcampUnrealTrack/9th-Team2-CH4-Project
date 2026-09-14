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
		// [09.14] 매크로 파싱 오류 방지를 위해 한 줄로 바인딩 해제
		BoundGameState->OnTeamScrapValueChanged.RemoveDynamic(this, &UBaruSharedAssetWidget::HandleTeamScrapValueChanged);
	}

	BoundGameState = nullptr;
	Super::NativeDestruct();
}

void UBaruSharedAssetWidget::BindGameState()
{
	if (IsValid(BoundGameState))
	{
		// [09.14] 매크로 파싱 오류 방지를 위해 한 줄로 바인딩 해제
		BoundGameState->OnTeamScrapValueChanged.RemoveDynamic(this, &UBaruSharedAssetWidget::HandleTeamScrapValueChanged);
	}

	BoundGameState = GetWorld() ? GetWorld()->GetGameState<ABaruGameState>() : nullptr;

	if (!IsValid(BoundGameState))
	{
		HandleTeamScrapValueChanged(0);
		return;
	}

	// [09.14] 매크로 내부 공백 생성 방지를 위해 반드시 한 줄로 바인딩
	BoundGameState->OnTeamScrapValueChanged.AddUniqueDynamic(this, &UBaruSharedAssetWidget::HandleTeamScrapValueChanged);

	// 위젯이 열리기 전에 변경된 값도 즉시 표시
	HandleTeamScrapValueChanged(BoundGameState->GetTeamScrapValue());
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