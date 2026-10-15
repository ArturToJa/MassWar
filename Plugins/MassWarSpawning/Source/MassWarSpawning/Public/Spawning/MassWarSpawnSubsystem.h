// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "MassEntityHandle.h"
#include "Spawning/MassWarSpawnTypes.h"
#include "MassWarSpawnSubsystem.generated.h"

class UMassWarFormationTemplate;

/**
 * The one place to create MassWar units. Spawns the units of a formation (in one batch per unit type), gives each its
 * team, owner, position and weapons, registers it, and groups them into a formation that can be selected and
 * ordered. Server / standalone only - on a client every call logs a warning and does nothing (clients receive units
 * through replication).
 *
 *   FMassWarFormationSpawnParams Params;
 *   Params.Units = { {RiflemanConfig, 20, {M4, Pistol}}, {GrenadierConfig, 5, {M4, Pistol, Rpg}} };
 *   Params.OwningPlayerId = 1; Params.TeamId = 1; Params.Location = ...;
 *   FMassWarSpawnedFormation Result = World->GetSubsystem<UMassWarSpawnSubsystem>()->SpawnFormation(Params);
 *
 * or, from a data asset: SpawnFormationFromTemplate(Template, PlayerId, TeamId, Location).
 */
UCLASS()
class MASSWARSPAWNING_API UMassWarSpawnSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Spawns a formation and its units (with their weapons). Returns the formation id and unit count; id 0 = failed (see log). */
	UFUNCTION(BlueprintCallable, Category = "MassWar|Spawning")
	FMassWarSpawnedFormation SpawnFormation(const FMassWarFormationSpawnParams& Params);

	/** Same, also returning the spawned entities (in roster order). C++ only. */
	FMassWarSpawnedFormation SpawnFormationWithUnits(const FMassWarFormationSpawnParams& Params, TArray<FMassEntityHandle>& OutUnits);

	/** Spawns the formation a template describes. */
	UFUNCTION(BlueprintCallable, Category = "MassWar|Spawning")
	FMassWarSpawnedFormation SpawnFormationFromTemplate(const UMassWarFormationTemplate* Template, int32 OwningPlayerId, int32 TeamId, FVector Location, float YawDegrees = 0.f);

	/** Spawns units that belong to no formation (scripted or neutral units; note that only formations can be selected and
	 *  ordered by players). Laid out in a grid around Location. Returns how many were spawned. */
	UFUNCTION(BlueprintCallable, Category = "MassWar|Spawning")
	int32 SpawnUnits(const FMassWarUnitSpawnEntry& Entry, int32 OwningPlayerId, int32 TeamId, FVector Location, float YawDegrees = 0.f, float Spacing = 130.f);

	/** As SpawnUnits, also returning the entities. C++ only. */
	int32 SpawnUnitsWithEntities(const FMassWarUnitSpawnEntry& Entry, int32 OwningPlayerId, int32 TeamId, const FVector& Location, float YawDegrees, float Spacing, TArray<FMassEntityHandle>& OutUnits);

private:
	/** Spawns every roster entry and places the units in one grid. */
	bool SpawnRoster(const TArray<FMassWarUnitSpawnEntry>& Roster, int32 OwningPlayerId, int32 TeamId, const FVector& Location, float YawDegrees, float Spacing, TArray<FMassEntityHandle>& OutUnits) const;

	bool CanSpawn(const TCHAR* Caller) const;
};
