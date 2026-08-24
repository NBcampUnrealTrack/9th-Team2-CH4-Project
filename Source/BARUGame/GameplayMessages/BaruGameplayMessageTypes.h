#pragma once

#include "CoreMinimal.h"

#include "BaruGameplayMessageTypes.generated.h"

/**
 * Minimal payload used while learning and validating the Gameplay Message Subsystem.
 * This is a transient local message, not replicated gameplay state.
 */
USTRUCT(BlueprintType)
struct BARUGAME_API FBaruDebugMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BARU|Gameplay Message")
	FString Text;

	UPROPERTY(BlueprintReadOnly, Category = "BARU|Gameplay Message")
	int32 Sequence = 0;
};
