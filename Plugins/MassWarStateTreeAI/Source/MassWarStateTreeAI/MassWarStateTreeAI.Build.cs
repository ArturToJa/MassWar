// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MassWarStateTreeAI : ModuleRules
{
	public MassWarStateTreeAI(ReadOnlyTargetRules Target) : base(Target)
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
				"MassSignals",
				"MassWar",
				"StateTreeModule",
				"MassAIBehavior",
			}
			);
	}
}
