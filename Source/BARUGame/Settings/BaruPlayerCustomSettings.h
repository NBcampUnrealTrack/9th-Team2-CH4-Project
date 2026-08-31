#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BaruPlayerCustomSettings.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBaruAudioVolumeChanged, FName, ChannelName, float, NewVolume);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBaruFOVChanged, float, NewFOV);

/**
 * 조작, 오디오, 1인칭 카메라, 공포 편의성 옵션
 */
UCLASS()
class BARUGAME_API UBaruPlayerCustomSettings : public USaveGame
{
	GENERATED_BODY()
	
public:
	UBaruPlayerCustomSettings();

	static FString GetSettingsSlotName() { return TEXT("PlayerCustomSettings_Default"); }

	// 설정 불러오기
	UFUNCTION(BlueprintCallable, Category = "BARU|Settings")
	static UBaruPlayerCustomSettings* LoadOrCreateSettings();

	// 현재 설정 저장
	UFUNCTION(BlueprintCallable, Category = "BARU|Settings")
	bool SaveCustomSettings();

	// 기본값 복원
	UFUNCTION(BlueprintCallable, Category = "BARU|Settings")
	void ResetToDefaults();

	// =========================================================
	// Control Settings
	// =========================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls", meta = (ClampMin = "0.01", ClampMax = "2.0"))
	float MouseSensitivity = 0.07f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls")
	bool bInvertMousePitch = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls")
	bool bInvertMouseYaw = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls")
	bool bEnableMouseSmoothing = true;

	// =========================================================
	// Audio Settings (0.0 ~ 1.0)
	// =========================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MasterVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BGMVolume = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SFXVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MonsterVolume = 1.0f;

	// =========================================================
	// Gameplay & Accessibility
	// =========================================================
	
	// FOV
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay", meta = (ClampMin = "70.0", ClampMax = "110.0"))
	float FieldOfView = 90.0f;

	// 1인칭 헤드보빙
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CameraShakeIntensity = 1.0f;

	// 화면 왜곡 효과 강도 (0.0: 왜곡 없음, 1.0: 기본. Sanity 저하 시 화면 연출 설정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SanityPostProcessScale = 1.0f;

	// 크로스헤어 표시
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	bool bShowCrosshair = true;

	// 자막 표시(자막 있을 경우)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	bool bEnableSubtitles = true;

public:
	UPROPERTY(BlueprintAssignable, Category = "BARU|Settings|Delegates")
	FOnBaruAudioVolumeChanged OnAudioVolumeChanged;

	UPROPERTY(BlueprintAssignable, Category = "BARU|Settings|Delegates")
	FOnBaruFOVChanged OnFOVChanged;
};
