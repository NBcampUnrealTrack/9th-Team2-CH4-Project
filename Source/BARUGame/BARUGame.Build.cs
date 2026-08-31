// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class BARUGame : ModuleRules
{
	public BARUGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PrivatePCHHeaderFile = "BARUGame.h";
		
		PublicIncludePaths.AddRange(new string[] {
			ModuleDirectory
		});
		
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"CommonUI",
			"GameplayAbilities",
			"GameplayMessageRuntime",
			"GameplayTags",
			"GameplayTasks",
			"NetCore",
			"HTTP",
			"Json",
			"JsonUtilities",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"OnlineSubsystemSteam",
			"SQLiteCore",
			"SQLiteSupport",
			"AIModule",
			"Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"CommonInput",
			"Slate",
			"SlateCore",
			"UMG"
		});
	}
}
