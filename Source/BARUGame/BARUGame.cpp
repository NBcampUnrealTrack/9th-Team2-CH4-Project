#include "BARUGame.h"
#include "Modules/ModuleManager.h"
#include "GameplayTags/BaruGameplayTags.h"
#include "BaruLog.h"
#include "HAL/PlatformMisc.h"
#include "Misc/ConfigCacheIni.h"

class FBARUGameModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FPlatformMisc::SetEnvironmentVar(TEXT("SteamAppId"), TEXT("480"));
		FPlatformMisc::SetEnvironmentVar(TEXT("SteamGameId"), TEXT("480"));

		// [핵심] ini 파일 파싱 실패를 원천 차단: 엔진 메모리에 로비 플래그를 직접 강제 설정
		if (GConfig)
		{
			GConfig->SetBool(TEXT("OnlineSubsystemSteam"), TEXT("bUseLobbiesIfAvailable"), true, GEngineIni);
			GConfig->SetBool(TEXT("OnlineSubsystemSteam"), TEXT("bUsesPresence"), true, GEngineIni);
			GConfig->SetBool(TEXT("OnlineSubsystemSteam"), TEXT("bFilterIncompatibleBuilds"), false, GEngineIni);
			GConfig->SetBool(TEXT("OnlineSubsystem"), TEXT("bFilterIncompatibleBuilds"), false, GEngineIni);
		}
       
		FBaruGameplayTags::InitializeNativeGameplayTags();
		BARU_LOG(LogBaruGAS, Log, TEXT("FBARUGameModule::StartupModule - Native GameplayTags Initialized."));
	}

	virtual void ShutdownModule() override
	{
		BARU_LOG(LogBaru, Log, TEXT("FBARUGameModule::ShutdownModule - Module Unloaded."));
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE( FBARUGameModule, BARUGame, "BARUGame" );