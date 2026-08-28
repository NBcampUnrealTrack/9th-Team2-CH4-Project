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

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
