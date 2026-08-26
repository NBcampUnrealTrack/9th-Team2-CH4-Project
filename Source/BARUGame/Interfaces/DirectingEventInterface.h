#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "DirectingEventInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UDirectingEventInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 비레벨 환경 연출 액터를 일괄 제어하기 위한 표준 인터페이스.
 */
class BARUGAME_API IDirectingEventInterface
{
	GENERATED_BODY()

public:

	// 지정된 태그에 해당하는 환경 연출 시작
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "BARU|Directing")
	void TriggerDirectingEvent(FGameplayTag EventTag, AActor* InstigatorActor);

	// 활성화된 환경 연출을 기본 상태로 복구하거나 중단
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "BARU|Directing")
	void ResetDirectingEvent(FGameplayTag EventTag);

	// 연출 진행중인지 여부를 반환
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "BARU|Directing")
	bool IsDirectingActive() const;

	// 연출 태그 반환
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "BARU|Directing")
	FGameplayTag GetDirectingTag() const;
};