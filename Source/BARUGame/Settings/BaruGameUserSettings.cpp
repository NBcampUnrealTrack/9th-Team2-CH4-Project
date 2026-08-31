#include "Settings/BaruGameUserSettings.h"
#include "Engine/Engine.h"
#include "BaruLog.h"

UBaruGameUserSettings::UBaruGameUserSettings()
{
	FrameRateLimitType = EBaruFrameRateLimit::FPS_60;
	DisplayGamma = 2.2f;
	bEnableMotionBlur = true;
}

UBaruGameUserSettings* UBaruGameUserSettings::GetBaruGameUserSettings()
{
	return Cast<UBaruGameUserSettings>(GEngine ? GEngine->GetGameUserSettings() : nullptr);
}

float UBaruGameUserSettings::FrameRateLimitToValue(EBaruFrameRateLimit InLimit)
{
	switch (InLimit)
	{
	case EBaruFrameRateLimit::FPS_30:  return 30.0f;
	case EBaruFrameRateLimit::FPS_60:  return 60.0f;
	case EBaruFrameRateLimit::FPS_120: return 120.0f;
	case EBaruFrameRateLimit::FPS_144: return 144.0f;
	default:                           return 60.0f;
	}
}

EBaruFrameRateLimit UBaruGameUserSettings::ValueToFrameRateLimit(float InValue)
{
	if (InValue <= 45.0f)
	{
		return EBaruFrameRateLimit::FPS_30;
	}
	if (InValue <= 90.0f)
	{
		return EBaruFrameRateLimit::FPS_60;
	}
	if (InValue <= 130.0f)
	{
		return EBaruFrameRateLimit::FPS_120;
	}
	return EBaruFrameRateLimit::FPS_144;
}

void UBaruGameUserSettings::SetFrameRateLimitType(EBaruFrameRateLimit InLimit)
{
	FrameRateLimitType = InLimit;
	const float TargetFPS = FrameRateLimitToValue(InLimit);
	
	SetFrameRateLimit(TargetFPS);
}

void UBaruGameUserSettings::SetDisplayGamma(float InGamma)
{
	DisplayGamma = FMath::Clamp(InGamma, 1.5f, 3.0f);
}

void UBaruGameUserSettings::SetMotionBlurEnabled(bool bEnable)
{
	bEnableMotionBlur = bEnable;
}

void UBaruGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();

	FrameRateLimitType = EBaruFrameRateLimit::FPS_60;
	DisplayGamma = 2.2f;
	bEnableMotionBlur = true;

	SetFrameRateLimit(FrameRateLimitToValue(FrameRateLimitType));
}

void UBaruGameUserSettings::ApplyNonResolutionSettings()
{
	Super::ApplyNonResolutionSettings();
	ApplyCustomGraphicsSettings();
}

void UBaruGameUserSettings::ApplyCustomGraphicsSettings()
{
	// 프레임 제한 수치 적용
	SetFrameRateLimit(FrameRateLimitToValue(FrameRateLimitType));

	// 감마 적용
	if (GEngine)
	{
		GEngine->DisplayGamma = DisplayGamma;
	}

	// 모션 블러 CVar 적용
	if (IConsoleVariable* CVarMotionBlur = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MotionBlurQuality")))
	{
		CVarMotionBlur->Set(bEnableMotionBlur ? 4 : 0, ECVF_SetByGameSetting);
	}

	BARU_LOG(LogBaru, Log, TEXT("BaruGameUserSettings Applied: MaxFPS=%.0f, Gamma=%.2f, MotionBlur=%d"),
		FrameRateLimitToValue(FrameRateLimitType), DisplayGamma, bEnableMotionBlur);
}