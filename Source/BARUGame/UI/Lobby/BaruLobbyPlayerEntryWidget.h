#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"

#include "BaruLobbyPlayerEntryWidget.generated.h"

class UTextBlock;

/** 로비 참가자 한 명을 표시하는 ListView Entry Widget */
UCLASS(Abstract, Blueprintable)
class BARUGAME_API UBaruLobbyPlayerEntryWidget
	: public UUserWidget,
	  public IUserObjectListEntry
{
	GENERATED_BODY()

protected:
	virtual void NativeOnListItemObjectSet(
		UObject* ListItemObject) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby|Player")
	TObjectPtr<UTextBlock> Text_PlayerName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby|Player")
	TObjectPtr<UTextBlock> Text_PlayerRole;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "BARU|UI|Lobby|Player")
	TObjectPtr<UTextBlock> Text_ReadyState;
};
