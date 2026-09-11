#include "Settings/BaruPlayerCustomSettings.h"
#include "Kismet/GameplayStatics.h"
#include "BaruLog.h"

UBaruPlayerCustomSettings::UBaruPlayerCustomSettings()
{
	ResetToDefaults();
}

UBaruPlayerCustomSettings* UBaruPlayerCustomSettings::LoadOrCreateSettings()
{
	const FString SlotName = GetSettingsSlotName();
	const int32 UserIndex = 0;

	if (UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex))
		{
			if (UBaruPlayerCustomSettings* Settings = Cast<UBaruPlayerCustomSettings>(Loaded))
			{
				BARU_LOG(LogBaruUI, Log, TEXT("PlayerCustomSettings Loaded successfully."));
				return Settings;
			}
		}
	}

	UBaruPlayerCustomSettings* NewSettings = Cast<UBaruPlayerCustomSettings>(
		UGameplayStatics::CreateSaveGameObject(UBaruPlayerCustomSettings::StaticClass()));
	
	if (NewSettings)
	{
		NewSettings->SaveCustomSettings();
		BARU_LOG(LogBaruUI, Log, TEXT("New PlayerCustomSettings Created and Saved."));
	}

	return NewSettings;
}

bool UBaruPlayerCustomSettings::SaveCustomSettings()
{
	const FString SlotName = GetSettingsSlotName();
	const bool bSuccess = UGameplayStatics::SaveGameToSlot(this, SlotName, 0);
	BARU_LOG(LogBaruUI, Log, TEXT("Save PlayerCustomSettings: %s"), bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));
	return bSuccess;
}

void UBaruPlayerCustomSettings::ResetToDefaults()
{
	MouseSensitivity = 0.07f;
	bInvertMousePitch = false;
	bInvertMouseYaw = false;
	bEnableMouseSmoothing = true;

	MasterVolume = 1.0f;
	BGMVolume = 0.8f;
	SFXVolume = 1.0f;
	MonsterVolume = 1.0f;

	FieldOfView = 90.0f;
	CameraShakeIntensity = 1.0f;
	SanityPostProcessScale = 1.0f;
	HitScreenEffectIntensity = 1.0f;
	bShowCrosshair = true;
	bEnableSubtitles = true;
}
