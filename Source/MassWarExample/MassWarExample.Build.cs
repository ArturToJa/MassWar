// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MassWarExample : ModuleRules
{
	public MassWarExample(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"MassWar", "MassWarCombat", "MassWarSelection", "MassWarStateTreeAI", "MassWarReplication", "MassWarFogOfWar", "MassWarEmbodiment", "MassEntity", "MassCommon", "MassSpawner", "MassRepresentation", "MassLOD", "MassActors" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Editor-only: the MassWarSetupStateTreeCommandlet builds/compiles the ST_MassWarUnit_Mass asset
		// in C++ (far more reliable than scripting the StateTree graph editor) - only needed/buildable
		// when this module is part of an Editor target.
		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd",
				"AssetRegistry",
				"StateTreeModule",
				"StateTreeEditorModule",
				"GameplayStateTreeModule",
				"MassAIBehavior",
				"PropertyBindingUtils",
				"MassReplication",
			});
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
