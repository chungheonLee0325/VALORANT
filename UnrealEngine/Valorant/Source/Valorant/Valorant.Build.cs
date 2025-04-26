// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Valorant : ModuleRules
{
	public Valorant(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[] {
			"Valorant" // Organizes the project into folders
		});
		
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput", 
			"GameplayAbilities", 
			"GameplayTags", 
			"GameplayTasks", 
			"UMG",
			"OnlineSubsystem",
			"OnlineSubsystemSteam",
			"OnlineSubsystemUtils",
			"HTTP",
			"Json",
			"MoviePlayer",
			"Slate", 
			"SlateCore"
		});
	}
}
