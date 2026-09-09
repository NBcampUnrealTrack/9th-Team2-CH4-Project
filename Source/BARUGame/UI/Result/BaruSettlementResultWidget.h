// BaruSettlementResultWiget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/BaruActivatableWidget.h"

#include "BaruSettlementResultWidget.generated.h"

class UTextBlock;

/**
 *	게임 종료 후 개인 정산 결과를 표시하는 위젯
 *	
 *	실제 결과 계산은 GameMode/PlayerController가 담당하고
 *	이 클래스는 전달받은 결과를 화면에 표시한다.
 */
UCLASS(Abstract, Blueprintable)
class BARUGAME_API UBaruSettlementResultWidget 
	: public UBaruActivatableWidget
{
	GENERATED_BODY()
	
public:
	UBaruSettlementResultWidget(
		const FObjectInitializer& ObjectInitializer);
	
	/**
	 *	결과창에 표시할 값을 전달한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "BARU|UI|Result")
	void SetResultData(
		bool bInSurvived,
		int32 InMonsterKillCount,
		int32 InExtractedItemCount,
		int32 InAcquiredCurrency);
	
protected:
	virtual void NativeOnActivated() override;
	
	// 저장된 결과를 TextBlock에 표시한다.
	void RefreshResultDisplay();
	
protected:
	// 임무 성공 또는 실패
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ResultTitle;
	
	// 처치한 몬스터 수
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_MonsterKillCount;
	
	// 파밍한 아이템 수
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ExtractedItemCount;
	
	// 획득한 재화
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_AcquiredCurrency;
	
private:
	bool bSurvived = false;
	
	int32 MonsterKillCount = 0;
	
	int32 ExtractedItemCount = 0;
	
	int32 AcquiredCurrency = 0;
};
