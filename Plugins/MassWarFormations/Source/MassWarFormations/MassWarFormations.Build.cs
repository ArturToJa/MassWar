// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MassWarFormations : ModuleRules
{
	public MassWarFormations(ReadOnlyTargetRules Target) : base(Target)
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
				"MassWarPerception",
			}
			);
	}
}
