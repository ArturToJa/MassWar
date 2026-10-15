// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MassWarSpawning : ModuleRules
{
	public MassWarSpawning(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"MassEntity",
				"MassCommon",
				"MassSpawner",
				"MassWar",
				"MassWarFormations",
				"MassWarWeapons",
			}
			);
	}
}
