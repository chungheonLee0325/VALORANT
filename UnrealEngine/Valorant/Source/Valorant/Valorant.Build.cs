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
			"CommonInput",
			"CommonUI",
			"ModelViewViewModel",
			"OnlineSubsystem",
			"OnlineSubsystemSteam",
			"OnlineSubsystemUtils",
			"PhysicsCore",
			"HTTP",
			"Json",
			"JsonUtilities",
			"MoviePlayer",
			"Slate", 
			"SlateCore",
			"Niagara"
		});
	}
}
