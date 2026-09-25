// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MassWarEmbodiment : ModuleRules
{
	public MassWarEmbodiment(ReadOnlyTargetRules Target) : base(Target)
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
				"MassActors",
				"MassLOD",
				"MassRepresentation",
				"MassWar",
			}
			);
	}
}
