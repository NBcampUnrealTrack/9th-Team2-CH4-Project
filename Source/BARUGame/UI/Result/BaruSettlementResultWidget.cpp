// BaruSettlimentResultWidget.cpp

#include "UI/Result/BaruSettlementResultWidget.h"

#include "BaruLog.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "BaruSettlementResultWidget"

UBaruSettlementResultWidget::UBaruSettlementResultWidget(
	const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
{
	// UI 입력만 받지만 게임 전체를 일시 정지하지는 않는다.
	InputConfig = EBaruWidgetInputMode::Menu;
}

void UBaruSettlementResultWidget::SetResultData(
	bool bInSurvived,
	int32 InMonsterKillCount,
	int32 InExtractedItemCount,
	int32 InAcquiredCurrency)
{
	bSurvived = bInSurvived;
	
	MonsterKillCount = FMath::Max(0, InMonsterKillCount);
	
	ExtractedItemCount = 
		FMath::Max(0, InExtractedItemCount);
	
	AcquiredCurrency =
		FMath::Max(0, InAcquiredCurrency);
	
	RefreshResultDisplay();
	
	BARU_LOG(
		LogBaruUI,
		Log,
		TEXT("정산 결과를 표시했습니다. " "Survived=%d, MonsterKills=%d, " "ExtractedItems=%d, Currency=%d"),
		bSurvived,
		MonsterKillCount,
		ExtractedItemCount,
		AcquiredCurrency);
}

void UBaruSettlementResultWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	
	RefreshResultDisplay();
}

void UBaruSettlementResultWidget::RefreshResultDisplay()
{
	if (IsValid(Text_ResultTitle))
	{
		Text_ResultTitle->SetText(
			bSurvived
				? LOCTEXT("MissionSuccess", "임무 성공")
				: LOCTEXT("MissionFailed", "임무 실패"));
	}
	
	if (IsValid(Text_MonsterKillCount))
	{
		Text_MonsterKillCount->SetText(
			FText::Format(
				LOCTEXT("MonsterKillCountFormat", "처치한 몬스터: {0}마리"),
				FText::AsNumber(MonsterKillCount)));
	}
	
	if (IsValid(Text_ExtractedItemCount))
	{
		Text_ExtractedItemCount->SetText(
			FText::Format(
				LOCTEXT("ExtractedItemCountFormat", "파밍한 아이템: {0}개"),
				FText::AsNumber(ExtractedItemCount)));
	}
	
	if (IsValid(Text_AcquiredCurrency))
	{
		Text_AcquiredCurrency->SetText(
			FText::Format(
				LOCTEXT("AcquiredCurrencyFormat", "획득 재화: {0}"),
				FText::AsNumber(AcquiredCurrency)));
	}
}

#undef LOCTEXT_NAMESPACE