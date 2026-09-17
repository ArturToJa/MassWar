// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MassWar : ModuleRules
{
	public MassWar(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"NetCore",
				"MassEntity",
				"MassCommon",
				"MassSpawner",
				"MassMovement",
				"MassRepresentation",
				"MassSimulation",
				"MassActors",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
			}
			);
	}
}
