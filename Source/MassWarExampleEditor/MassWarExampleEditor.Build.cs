// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

/** Editor-only tools for the MassWarExample project (asset setup commandlets). Never part of game or dedicated server builds. */
public class MassWarExampleEditor : ModuleRules
{
	public MassWarExampleEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "UnrealEd", "AssetRegistry",
			"MassEntity", "MassSpawner", "MassReplication", "MassAIBehavior",
			"StateTreeModule", "StateTreeEditorModule", "GameplayStateTreeModule", "PropertyBindingUtils",
			"MassWar", "MassWarStateTreeAI", "MassWarFogOfWar", "MassWarReplication", "MassWarEmbodiment",
		});
	}
}
