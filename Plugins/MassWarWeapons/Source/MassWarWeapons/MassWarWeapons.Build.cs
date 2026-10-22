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
		// Since UE 5.8 the core Mass types (FMassFragment, FMassEntityHandle, FTransformFragment...) live in their own module.
		if (Target.Version.MajorVersion > 5 || Target.Version.MinorVersion >= 8)
		{
			PublicDependencyModuleNames.Add("MassCore");
		}
	}
}
