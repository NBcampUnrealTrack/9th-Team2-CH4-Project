#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "BaruLobbyPlayerListItemData.generated.h"

/**
 * 로비 참가자 목록 한 줄에 표시할 UI 전용 데이터
 *
 * 실제 네트워크 상태는 ABaruPlayerState가 소유하고,
 * 이 객체는 ListView에 전달할 화면 표시용 스냅샷만 보관한다.
 */
UCLASS(BlueprintType)
class BARUGAME_API UBaruLobbyPlayerListItemData : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(
		const FString& InPlayerName,
		bool bInIsHost,
		bool bInIsReady,
		bool bInIsLocalPlayer);

	UPROPERTY(BlueprintReadOnly, Category = "BARU|UI|Lobby|Player")
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "BARU|UI|Lobby|Player")
	bool bIsHost = false;

	UPROPERTY(BlueprintReadOnly, Category = "BARU|UI|Lobby|Player")
	bool bIsReady = false;

	UPROPERTY(BlueprintReadOnly, Category = "BARU|UI|Lobby|Player")
	bool bIsLocalPlayer = false;
};
