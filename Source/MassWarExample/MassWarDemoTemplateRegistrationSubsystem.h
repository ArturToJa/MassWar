// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MassWarDemoTemplateRegistrationSubsystem.generated.h"

/**
 * Pre-registers DA_MassWarDemoUnit's Mass entity template in every world, regardless of net mode.
 * AMassWarDemoGameMode already does this incidentally when it spawns the two teams (SpawnTeam calls
 * UMassEntityConfigAsset::GetOrCreateEntityTemplate) - but GameMode is server-only (it never exists on a
 * client), so a client world's UMassSpawnerSubsystem never registers the template on its own. When
 * MassWarReplication then tries to spawn a replicated proxy entity on the client via
 * UMassSpawnerSubsystem::SpawnEntities(TemplateID, ...), TemplateID (deterministic from the config
 * asset's own GUID - identical on server and client) was never registered locally, and the engine
 * asserts ("TemplateID must have been registered!"). This subsystem exists on every net mode/world and
 * closes that gap; not part of any MassWar plugin since it only matters for this demo's specific asset.
 */
UCLASS()
class MASSWAREXAMPLE_API UMassWarDemoTemplateRegistrationSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

protected:
	virtual void PostInitialize() override;
};
