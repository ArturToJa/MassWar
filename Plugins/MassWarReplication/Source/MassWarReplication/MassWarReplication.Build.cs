// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MassWarReplication : ModuleRules
{
	public MassWarReplication(ReadOnlyTargetRules Target) : base(Target)
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
				"MassReplication",
				"MassLOD",
				"MassWar",
			}
			);
	}
}
