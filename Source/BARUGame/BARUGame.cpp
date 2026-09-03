// Copyright Epic Games, Inc. All Rights Reserved.

#include "BARUGame.h"
#include "Modules/ModuleManager.h"
#include "GameplayTags/BaruGameplayTags.h"
#include "BaruLog.h"

/**
 * Primary Game Module
 */
class FBARUGameModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FBaruGameplayTags::InitializeNativeGameplayTags();
		BARU_LOG(LogBaruGAS, Log, TEXT("FBARUGameModule::StartupModule - Native GameplayTags Initialized."));
	}

	virtual void ShutdownModule() override
	{
		BARU_LOG(LogBaru, Log, TEXT("FBARUGameModule::ShutdownModule - Module Unloaded."));
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE( FBARUGameModule, BARUGame, "BARUGame" );
