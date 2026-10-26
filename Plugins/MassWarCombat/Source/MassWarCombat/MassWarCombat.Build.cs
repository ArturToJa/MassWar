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
				"DeveloperSettings",
			}
			);
		// Since UE 5.8 the core Mass types (FMassFragment, FMassEntityHandle, FTransformFragment...) live in their own module.
		if (Target.Version.MajorVersion > 5 || Target.Version.MinorVersion >= 8)
		{
			PublicDependencyModuleNames.Add("MassCore");
		}
	}
}
