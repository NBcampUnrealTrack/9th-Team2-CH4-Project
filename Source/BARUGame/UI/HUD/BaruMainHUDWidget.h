// BaruMainHUDWidget.h

#pragma once

#include "CoreMinimal.h"
#include "UI/Foundation/BaruActivatableWidget.h"

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
	
	// 로컬 플레이어의 PlayerState와 HUD를 연결한다.
	void BindToPlayerState();
	
	// 등록했던 체력, 정신력 델리게이트를 해제한다.
	void UnbindFromPlayerState();
	
	// 현재 PlayerState 값을 HUD에 한 번에 표시한다.
	void RefreshPlayerStatus();
	
	// 체력 ProgressBar와 숫자 Text를 갱신한다.
	void UpdateHealthDisplay();
	
	// 정신력 ProgressBar와 숫자 Text를 갱신한다.
	void UpdateSanityDisplay();
	
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
	
	// 체력 게이지
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_Health;
	
	// 현재 체력 / 최대 체력
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_HealthValue;
	
	// 정신력 게이지
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_Sanity;
	
	// 현재 정신력
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SanityValue;
	
	// 상호작용 안내 전체 영역
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> Border_InteractionPrompt;
	
	// 상호작용 안내 문구
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_InteractionPrompt;
	
	// 현재 HUD가 관찰하고 잇는 로컬 BoundPlayerState;
	UPROPERTY(Transient)
	TObjectPtr<ABaruPlayerState> BoundPlayerState;
	
	// 현재 HUD가 관찰하고 있는 체력 컴포넌트
	UPROPERTY(Transient)
	TObjectPtr<UBaruHealthComponent> BoundHealthComponent;
	
	// [08.30] 포커스 타깃을 nullptr로 돌려 CommonUI의 Slate 포커스 강탈 방지
	virtual UWidget* NativeGetDesiredFocusTarget() const override { return nullptr; }
};
