// BaruSharedAssetWidget.cpp


#include "UI/InventoryUI/BaruSharedAssetWidget.h"
#include "Components/TextBlock.h"
#include "Core/BaruGameState.h"
#include "Engine/World.h"
#include "Subsystems/BaruSaveGameSubsystem.h"
#include "Engine/GameInstance.h"

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
		BoundGameState->OnTeamScrapValueChanged.RemoveDynamic(this, &UBaruSharedAssetWidget::HandleTeamScrapValueChanged);
	}

	BoundGameState = GetWorld() ? GetWorld()->GetGameState<ABaruGameState>() : nullptr;

	// [핵심 수정] 로비 레벨일 때는 세이브 서브시스템에 기록된 누적 골드를 표시
	if (GetWorld() && GetWorld()->GetMapName().Contains(TEXT("Lobby")))
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UBaruSaveGameSubsystem* SaveSubsystem = GI->GetSubsystem<UBaruSaveGameSubsystem>())
			{
				if (UBaruSaveGame* SaveData = SaveSubsystem->GetCachedSaveGame())
				{
					HandleTeamScrapValueChanged(SaveData->TotalGold);
					return;
				}
			}
		}
	}

	if (!IsValid(BoundGameState))
	{
		HandleTeamScrapValueChanged(0);
		return;
	}

	BoundGameState->OnTeamScrapValueChanged.AddUniqueDynamic(this, &UBaruSharedAssetWidget::HandleTeamScrapValueChanged);
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