// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MassWarPerception : ModuleRules
{
	public MassWarPerception(ReadOnlyTargetRules Target) : base(Target)
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
			}
			);
	}
}
