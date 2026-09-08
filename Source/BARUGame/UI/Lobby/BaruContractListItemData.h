// BaruContractListItemData.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "BaruContractListItemData.generated.h"

class UTexture2D;
class UBaruContractListItemData;

/**
 * 블루프린트에서 설정할 수 있는 계약 원본 데이터
 */
USTRUCT(BlueprintType)
struct BARUGAME_API FBaruContractDefinition
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|UI|Lobby|Contract")
	FText ContractName;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|UI|Lobby|Contract")
	FText MapName;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|UI|Lobby|Contract")
	FText Difficulty;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|UI|Lobby|Contract")
	FText RewardText;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|UI|Lobby|Contract")
	FString TargetMapURL;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BARU|UI|Lobby|Contract")
	TObjectPtr<UTexture2D> Thumbnail = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnBaruContractSelected,
	UBaruContractListItemData*,
	SelectedContract
	);

/**
 * 계약 목록 한 항목의 UI 표시 데이터를 보관한다.
 * 
 * 실제 계약 DataAsset이 추가되기 전까지
 *  ListView에 전달하는 UI용 데이터 객체로 사용한다.
 */
UCLASS(BlueprintType)
class BARUGAME_API UBaruContractListItemData : public UObject
{
	GENERATED_BODY()
	
public:
	void Initialize(
		const FText& InContractName,
		const FText& InMapName,
		const FText& InDifficulty,
		const FText& InRewardText,
		const FString& InTargetMapURL,
		UTexture2D* InThumbnail
		);
	
	UFUNCTION(BlueprintCallable, Category = "BARU|UI|Lobby|Contract")
	void RequestSelection();
	
	UPROPERTY(BlueprintReadOnly, Category = "BARU|UI|Lobby|Contract")
	FText ContractName;
	
	UPROPERTY(BlueprintReadOnly, Category = "BARU|UI|Lobby|Contract")
	FText MapName;
	
	UPROPERTY(BlueprintReadOnly, Category = "BARU|UI|Lobby|Contract")
	FText Difficulty;
	
	UPROPERTY(BlueprintReadOnly, Category = "BARU|UI|Lobby|Contract")
	FText RewardText;
	
	// 실제 게임 시작 시 이동할 맵 경로
	UPROPERTY(BlueprintReadOnly, Category = "BARU|UI|Lobby|Contract")
	FString TargetMapURL;
	
	UPROPERTY(BlueprintReadOnly, Category = "BARU|UI|Lobby|Contract")
	TObjectPtr<UTexture2D> Thumbnail;
	
	UPROPERTY(BlueprintAssignable, Category = "BARU|UI|Lobby|Contract")
	FOnBaruContractSelected OnSelected;
};