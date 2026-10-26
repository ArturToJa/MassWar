// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Weapons/MassWarWeaponSubsystem.h"
#include "Formation/MassWarFormationTypes.h"
#include "MassWarSpawnTypes.generated.h"

class UMassEntityConfigAsset;
class UMassWarStatSet;

/**
 * "Spawn this many units of this type, carrying these weapons." One line of a formation's roster.
 * The Config is the project's own Mass entity config for the unit type: it must contain the "MassWar Unit" trait
 * (team/owner/life/...), and the "MassWar Weapons" trait if Loadout is used.
 */
USTRUCT(BlueprintType)
struct MASSWARSPAWNING_API FMassWarUnitSpawnEntry
{
	GENERATED_BODY()

	/** The unit type: a Mass entity config asset (e.g. the project's DA_Rifleman). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Spawning")
	TObjectPtr<UMassEntityConfigAsset> Config;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Spawning", meta = (ClampMin = "1"))
	int32 Count = 1;

	/** The weapons every unit of this entry carries (any slot may be empty; the first non-empty slot starts in hand).
	 *  Weapons are not separate things to spawn: a unit's weapons are just entries in its loadout. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Spawning")
	FMassWarLoadout Loadout;

	/** Stat sets applied to every unit of this entry from the start (upgrades, veterancy...; at most 4). They must be listed in
	 *  Project Settings > MassWar Stats > Stat Set Catalog - that is how clients learn about them - and they replicate like any run-time stat set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Spawning")
	TArray<TObjectPtr<UMassWarStatSet>> StatSets;
};

/** Everything needed to spawn one formation: its roster, who owns it, and where it appears. */
USTRUCT(BlueprintType)
struct MASSWARSPAWNING_API FMassWarFormationSpawnParams
{
	GENERATED_BODY()

	/** The units the formation owns, in order. All of them are spawned and become the formation's members. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Spawning")
	TArray<FMassWarUnitSpawnEntry> Units;

	/** Shape, spacing, auto-engage... of the formation (the spacing also sets how far apart the units start out). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Spawning")
	FMassWarFormationSettings Settings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Spawning")
	int32 OwningPlayerId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Spawning")
	int32 TeamId = 0;

	/** Centre of the formation. Z is where the units stand. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Spawning")
	FVector Location = FVector::ZeroVector;

	/** Direction the formation faces, in degrees around Z. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Spawning")
	float YawDegrees = 0.f;
};

/** What a spawn call produced. */
USTRUCT(BlueprintType)
struct MASSWARSPAWNING_API FMassWarSpawnedFormation
{
	GENERATED_BODY()

	/** The id used by orders and selection (0 = nothing was spawned). */
	UPROPERTY(BlueprintReadOnly, Category = "MassWar|Spawning")
	int32 FormationId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "MassWar|Spawning")
	int32 NumUnits = 0;

	bool IsValid() const { return FormationId != 0; }
};
