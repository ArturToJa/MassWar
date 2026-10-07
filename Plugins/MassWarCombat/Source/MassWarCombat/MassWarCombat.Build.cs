// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MassWarCombat : ModuleRules
{
	public MassWarCombat(ReadOnlyTargetRules Target) : base(Target)
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
