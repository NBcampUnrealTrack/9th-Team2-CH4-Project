// BaruAllStatusEntryWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"

#include "BaruAllyStatusEntryWidget.generated.h"

class ABaruPlayerState;
class UBaruHealthComponent;
class UProgressBar;
class UTextBlock;

/**
 *	인게임 HUD에서 아군 한 명의 상태를 표시하는 ListView Entry.
 */
UCLASS(Abstract, Blueprintable)
class BARUGAME_API UBaruAllyStatusEntryWidget : public UUserWidget,
	public IUserObjectListEntry
{
	GENERATED_BODY()
	
protected:
	virtual void NativeOnListItemObjectSet(
		UObject* ListItemObject) override;
	
	virtual void NativeDestruct() override;
	
	// 기존 PlayerState의 델리게이트 연결을 해제한다.
	void UnbindFromPlayerState();
	
	// 이름, 체력, 정신력, 생존 상태를 모두 갱신한다.
	void RefreshDisplay();
	
	UFUNCTION()
	void HandleHealthChanged(
		UBaruHealthComponent* HealthComponent,
		float OldHealth,
		float NewHealth,
		AActor* Instigator);
	
	UFUNCTION()
	void HandleMaxHealthChanged(
		float OldMaxHealth,
		float NewMaxHealth);
	
	UFUNCTION()
	void HandleSanityChanged(float NewSanity);
	
	UFUNCTION()
	void HandleDBNOStatusChanged(bool bIsDBNO);
	
	UFUNCTION()
	void HandleDeadStatusChanged(bool bIsDead);
	
protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_AllyName;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_AllyState;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_AllyHealth;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_AllySanity;
	
private:
	UPROPERTY(Transient)
	TObjectPtr<ABaruPlayerState> BoundPlayerState;
	
	UPROPERTY(Transient)
	TObjectPtr<UBaruHealthComponent> BoundHealthComponent;
};