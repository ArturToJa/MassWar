// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

/**
 * Dedicated server: headless, authoritative simulation (Core movement/navigation/avoidance, Combat, Perception,
 * Formations, StateTree AI, FogOfWar visibility, Replication). Client-only presentation (selection input/HUD, puppets,
 * ISM updates, ghosts) is excluded at runtime by the processors' and subsystems' own net-mode checks.
 */
public class MassWarExampleServerTarget : TargetRules
{
	public MassWarExampleServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("MassWarExample");
	}
}
