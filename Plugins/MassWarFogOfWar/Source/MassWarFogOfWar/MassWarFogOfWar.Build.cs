// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MassWarFogOfWar : ModuleRules
{
	public MassWarFogOfWar(ReadOnlyTargetRules Target) : base(Target)
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
				"MassLOD",
				"MassRepresentation",
				"MassWar",
				"MassWarReplication",
			}
			);
	}
}
