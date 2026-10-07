// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MassWarSelection : ModuleRules
{
	public MassWarSelection(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"NetCore",
				"MassEntity",
				"MassCommon",
				"MassSpawner",
				"MassWar",
				"MassWarFormations",
			}
			);
	}
}
