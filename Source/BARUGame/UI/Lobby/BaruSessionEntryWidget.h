// BaruSessionEntryWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "UI/Foundation/BaruCommonUserWidget.h"

#include "BaruSessionEntryWidget.generated.h"

class UButton;
class UTextBlock;
class UBaruSessionListItemData;

/**
 * 세션 검색 결과 한 건을 표시하는 ListView Entry Widget
 * 
 * ListView가 Item Object를 할당하면
 * 해당 세션의 방 이름, 방장, 맵, 인원, Ping을 화면에 표시한다.
 */
UCLASS(Abstract, Blueprintable)
class BARUGAME_API UBaruSessionEntryWidget
	: public UBaruCommonUserWidget,
	public IUserObjectListEntry
{
	GENERATED_BODY()
	
protected:
	virtual void NativeOnInitialized() override;
	
	virtual void NativeOnListItemObjectSet(
		UObject* ListItemObject
		) override;
	
	UFUNCTION()
	void HandleJoinSessionClicked();
	
	// 검색 결과 표시 위젯
	UPROPERTY(
		BlueprintReadOnly,
		meta = (BindWidget),
		Category = "BARU|UI|Lobby|Session")
	TObjectPtr<UTextBlock> Text_ServerName;

	UPROPERTY(
		BlueprintReadOnly,
		meta = (BindWidget),
		Category = "BARU|UI|Lobby|Session")
	TObjectPtr<UTextBlock> Text_HostPlayerName;
	
	UPROPERTY(
		BlueprintReadOnly,
		meta = (BindWidget),
		Category = "BARU|UI|Lobby|Session")
	TObjectPtr<UTextBlock> Text_SelectedMapName;
	
	UPROPERTY(
		BlueprintReadOnly,
		meta = (BindWidget),
		Category = "BARU|UI|Lobby|Session")
	TObjectPtr<UTextBlock> Text_PlayerCount;
	
	UPROPERTY(
		BlueprintReadOnly,
		meta = (BindWidget),
		Category = "BARU|UI|Lobby|Session")
	TObjectPtr<UTextBlock> Text_Ping;
	
	UPROPERTY(
		BlueprintReadOnly,
		meta = (BindWidget),
		Category = "BARU|UI|Lobby|Session")
	TObjectPtr<UButton> Button_JoinSession;
	
	// 현재 이 Entry가 표시 중인 세션 데이터
	UPROPERTY(Transient)
	TObjectPtr<UBaruSessionListItemData> ItemData;
};
