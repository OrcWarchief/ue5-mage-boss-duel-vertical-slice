// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MageBossDuel : ModuleRules
{
	public MageBossDuel(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"MageBossDuel",
			"MageBossDuel/Variant_Platforming",
			"MageBossDuel/Variant_Platforming/Animation",
			"MageBossDuel/Variant_Combat",
			"MageBossDuel/Variant_Combat/AI",
			"MageBossDuel/Variant_Combat/Animation",
			"MageBossDuel/Variant_Combat/Gameplay",
			"MageBossDuel/Variant_Combat/Interfaces",
			"MageBossDuel/Variant_Combat/UI",
			"MageBossDuel/Variant_SideScrolling",
			"MageBossDuel/Variant_SideScrolling/AI",
			"MageBossDuel/Variant_SideScrolling/Gameplay",
			"MageBossDuel/Variant_SideScrolling/Interfaces",
			"MageBossDuel/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
