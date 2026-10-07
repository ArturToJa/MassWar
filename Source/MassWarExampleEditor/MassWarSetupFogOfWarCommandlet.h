// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MassWarSetupFogOfWarCommandlet.generated.h"

/**
 * One-off setup tool (run via `UnrealEditor-Cmd.exe <uproject> -run=MassWarSetupFogOfWar`): adds a
 * UMassWarVisibilityTrait to DA_MassWarDemoUnit so every spawned unit can act as a sight source for its
 * team. Mirrors MassWarSetupReplicationCommandlet's pattern. Project-specific demo tooling, not part of
 * any MassWar plugin.
 */
UCLASS()
class UMassWarSetupFogOfWarCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
