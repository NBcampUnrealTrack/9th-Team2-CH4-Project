// BaruSessionListItemData.h

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/BaruSessionSubsystem.h"

#include "BaruSessionListItemData.generated.h"

/**
 * ListView의 방 검색 결과 한 줄을 나타내는 데이터 객체
 * 
 * FBaruSessionSearchResultInfo는 구조체이므로
 * UObject를 요구하는 ListView에 직접 추가할 수 없다.
 * 따라서 검색 결과 구조체를 이 객체에 담아 ListView Item으로 사용한다.
 */
UCLASS(BlueprintType)
class BARUGAME_API UBaruSessionListItemData : public UObject
{
	GENERATED_BODY()
	
public:
	void Initialize(
		const FBaruSessionSearchResultInfo& InSessionInfo
		);
	
	UPROPERTY(BlueprintReadOnly, Category = "BARU|UI|Lobby|Session")
	FBaruSessionSearchResultInfo SessionInfo;
};