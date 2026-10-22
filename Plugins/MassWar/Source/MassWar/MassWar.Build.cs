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
				"GameplayTags",
				"DeveloperSettings",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"NavigationSystem",
			}
			);
		// Since UE 5.8 the core Mass types (FMassFragment, FMassEntityHandle, FTransformFragment...) live in their own module.
		if (Target.Version.MajorVersion > 5 || Target.Version.MinorVersion >= 8)
		{
			PublicDependencyModuleNames.Add("MassCore");
		}
	}
}
