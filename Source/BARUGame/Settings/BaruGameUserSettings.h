#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "BaruGameUserSettings.generated.h"

// 프레임 제한 ENUM 열거형
UENUM(BlueprintType)
enum class EBaruFrameRateLimit : uint8
{
	FPS_30   UMETA(DisplayName = "30 FPS"),
	FPS_60   UMETA(DisplayName = "60 FPS"),
	FPS_120  UMETA(DisplayName = "120 FPS"),
	FPS_144  UMETA(DisplayName = "144 FPS")
};

/**
 * 하드웨어 및 렌더링 성능 설정
 */
UCLASS(Config = GameUserSettings)
class BARUGAME_API UBaruGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()
	
public:
	UBaruGameUserSettings();
	
	UFUNCTION(BlueprintPure, Category = "BARU|Settings")
	static UBaruGameUserSettings* GetBaruGameUserSettings();

	virtual void ApplyNonResolutionSettings() override;
	virtual void SetToDefaults() override;
	
	
	
	// Frame Rate Limit (30, 60, 120, 144 FPS)
	UFUNCTION(BlueprintPure, Category = "BARU|Settings|Graphics")
	EBaruFrameRateLimit GetFrameRateLimitType() const { return FrameRateLimitType; }

	UFUNCTION(BlueprintCallable, Category = "BARU|Settings|Graphics")
	void SetFrameRateLimitType(EBaruFrameRateLimit InLimit);

	// Enum 값을 실제 float 프레임 수치로 변환
	UFUNCTION(BlueprintPure, Category = "BARU|Settings|Graphics")
	static float FrameRateLimitToValue(EBaruFrameRateLimit InLimit);

	// float 수치를 가장 근접한 Enum 값으로 역변환
	UFUNCTION(BlueprintPure, Category = "BARU|Settings|Graphics")
	static EBaruFrameRateLimit ValueToFrameRateLimit(float InValue);
	
	
	
	// Gamma & Display
	UFUNCTION(BlueprintPure, Category = "BARU|Settings|Display")
	float GetDisplayGamma() const { return DisplayGamma; }

	UFUNCTION(BlueprintCallable, Category = "BARU|Settings|Display")
	void SetDisplayGamma(float InGamma);

	
	

	// Rendering & Visual Quality
	UFUNCTION(BlueprintPure, Category = "BARU|Settings|Graphics")
	bool IsMotionBlurEnabled() const { return bEnableMotionBlur; }

	UFUNCTION(BlueprintCallable, Category = "BARU|Settings|Graphics")
	void SetMotionBlurEnabled(bool bEnable);

	
	
protected:
	// 선택된 프레임 제한 타입
	UPROPERTY(Config)
	EBaruFrameRateLimit FrameRateLimitType = EBaruFrameRateLimit::FPS_60;
	
	// 화면 밝기(기본값 설정)
	UPROPERTY(Config)
	float DisplayGamma = 2.2f;

	// 모션 블러 활성화 여부
	UPROPERTY(Config)
	bool bEnableMotionBlur = true;


private:
	void ApplyCustomGraphicsSettings();
};
