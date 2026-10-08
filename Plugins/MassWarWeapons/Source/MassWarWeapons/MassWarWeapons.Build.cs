// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MassWarWeapons : ModuleRules
{
	public MassWarWeapons(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"DeveloperSettings",
				"Niagara",
				"MassEntity",
				"MassCommon",
				"MassSpawner",
				"MassWar",
			}
			);
	}
}
