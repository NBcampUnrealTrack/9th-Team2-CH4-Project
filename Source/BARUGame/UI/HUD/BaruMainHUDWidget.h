// BaruMainHUDWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/BaruActivatableWidget.h"

#include "TimerManager.h"

#include "BaruMainHUDWidget.generated.h"

class ABaruPlayerState;
class UBaruHealthComponent;
class UProgressBar;
class UTextBlock;
class UBorder;

/**
 * 플레이 중 항상 표시되는 Main HUD의 C++ 기반 클래스,
 *
 * 이후 체력, 정신력, 크로스헤어, 상호작용 안내 등의
 * HUD 요소를 포함하는 최상위 위젯으로 사용한다.
 */
UCLASS(Abstract, Blueprintable)
class BARUGAME_API UBaruMainHUDWidget
	: public UBaruActivatableWidget
{
	GENERATED_BODY()

public:
	UBaruMainHUDWidget(
		const FObjectInitializer& ObjectInitializer);
	
	/**
	 * 상호작용 가능한 대상을 바라볼 떄
	 * 지정한 안내 문구를 화면에 표시한다.
	 * 
	 * 예: "[E] 문 열기", "[E] 아이템 줍기"
	 */
	UFUNCTION(BlueprintCallable, Category = "BARU|UI|HUD")
	void ShowInteractionPrompt(const FText& PromptText);
	
	// 화면 상단에 안내 메시지를 표시한다.
	UFUNCTION(BlueprintCallable, Category = "BARU|UI|HUD")
	void ShowGuideMessage(
		const FText& Message,
		float Duration = 3.0f);
	
	// 현재 안내 메시지를 숨긴다.
	UFUNCTION(BlueprintCallable, Category = "BARU|UI|HUD")
	void HideGuideMessage();
	
	/**
	 * 현재 장착한 무기와 탄약 정보를 표시한다.
	 */
	UFUNCTION(BlueprintCallable, Category = "BARU|UI|HUD")
	void ShowWeaponDisplay(
		const FText& WeaponName,
		int32 CurrentAmmo,
		int32 ReserveAmmo);
	
	/**
	 * 무기 정보 UI를 숨긴다.
	 */
	UFUNCTION(BlueprintCallable, Category = "BARU|UI|HUD")
	void HideWeaponDisplay();
	
	/**
	 * 현재 표시 중인 상호작용 안내를 숨긴다.
	 */
	UFUNCTION(BlueprintCallable, Category = "BARU|UI|HUD")
	void HideInteractionPrompt();
	
	// [08.30] CommonUI가 포커스를 요구하지 않도록 비활성화, 무조건 1인칭 Game 전용 모드 반환
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override
	{
		return FUIInputConfig(ECommonInputMode::Game, EMouseCaptureMode::CapturePermanently, true);
	}

protected:
	/**
	 * Main HUD가 활성화 될 때 호출된다.
	 *
	 * 이후 ViewModel 연결, Delegate 등록,
	 * 초기 데이터 갱신 등에 사용한다.
	 */
	virtual void NativeOnActivated() override;

	/**
	 * Main HUD가 비활성화될 때 호출된다.
	 *
	 * 이후 Delegate와 Listener 해제 등에 사용한다.
	 */
	virtual void NativeOnDeactivated() override;
	
	/**
	 * Mian HUD가 Slate 포커스를 가져가지 않도록 한다.
	 */
	virtual UWidget* NativeGetDesiredFocusTarget() const override
	{
		return nullptr;
	}
	
	// 로컬 플레이어의 PlayerState와 HUD를 연결한다.
	void BindToPlayerState();
	
	// 등록했던 체력, 정신력 DeLegate를 해제한다.
	void UnbindFromPlayerState();
	
	// 현재 플레이어 상태를 HUD에 한 번에 표시한다.
	void RefreshPlayerStatus();
	
	// 체력 ProgressBar와 수치 Text를 갱신한다.
	void UpdateHealthDisplay();
	
	// 정신력 ProgressBar와 수치 Text를 갱신한다.
	void UpdateSanityDisplay();
	
	// 현재 체력이 변경됐을 때 호출된다.
	UFUNCTION()
	void HandleHealthChanged(
		UBaruHealthComponent* HealthComponent,
		float OldHealth,
		float NewHealth,
		AActor* Instigator);
	
	// 최대 체력이 변경됐을 때 호출된다.
	UFUNCTION()
	void HandleMaxHealthChanged(
		float OldMaxHealth,
		float NewMaxHealth);
	
	// 정신력이 변경됐을 때 호출된다.
	UFUNCTION()
	void HandleSanityChanged(float NewSanity);
	
protected:
	// 체력 게이지
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_Health;
	
	// 현재 체력 / 최대 체력
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_HealthValue;
	
	// 정신력 게이지
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_Sanity;
	
	// 현재 정신력 / 최대 정신력
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SanityValue;
	
	// 상호작용 안내 전체 영역
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Border_InteractionPrompt;
	
	// 상호작용 안내 문구
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_InteractionPrompt;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Border_GuideMessage;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_GuideMessage;
	
	// 무기와 탄약 정보 전체 영역
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Border_WeaponStatus;
	
	// 현재 장착한 무기 이름
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_WeaponName;
	
	// 탄창에 남은 탄약
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_CurrentAmmo;
	
	// 보유 중인 예비 탄약
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ReserveAmmo;
	
private:
	// 현재 HUD가 관찰하고있는 로컬 PlayerState
	UPROPERTY(Transient)
	TObjectPtr<ABaruPlayerState> BoundPlayerState;
	
	// 현재 HUD가 관찰하고 있는 체력 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<UBaruHealthComponent> BoundHealthComponent;
	
	FTimerHandle GuideMessageTimerHandle;
};
