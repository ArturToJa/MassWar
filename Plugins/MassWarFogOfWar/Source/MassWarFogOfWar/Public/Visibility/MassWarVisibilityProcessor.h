// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassWarVisibilityProcessor.generated.h"

/**
 * Server-only: every UpdateInterval seconds works out, per team, which enemy units that team can currently
 * see, and publishes it to UMassWarVisibilitySubsystem. A unit is visible to a team if one of the team's own
 * units has it within its sight radius and (bRequireLineOfSight) a clear line of sight to it.
 * Simulation itself never consults this - every unit's AI/combat/movement keeps running regardless of
 * visibility; this only gates what gets replicated to clients (see UMassWarVisibilitySubsystem::IsRelevant).
 *
 * Built for big crowds: a hashed grid finds the viewers near each candidate (no all-pairs loop), and line of
 * sight only tests the few nearest viewers, caches its answer for a moment and stays inside a shared
 * raycast budget (UMassWarLineOfSightSubsystem) - when the budget runs out a unit keeps its last answer.
 */
UCLASS()
class MASSWARFOGOFWAR_API UMassWarVisibilityProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassWarVisibilityProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	UPROPERTY(EditAnywhere, Category = "MassWar|FogOfWar")
	float UpdateInterval = 0.2f;

	/** Sight also needs an unobstructed line (level geometry blocks it). Off = radius only, as before. */
	UPROPERTY(EditAnywhere, Category = "MassWar|FogOfWar")
	bool bRequireLineOfSight = true;

	/** At most this many of the nearest in-range viewers are ray-tested per unit per update. */
	UPROPERTY(EditAnywhere, Category = "MassWar|FogOfWar", meta = (EditCondition = "bRequireLineOfSight", ClampMin = "1"))
	int32 MaxLineOfSightTestsPerUnit = 3;

	/** A line-of-sight answer is reused for this long (seconds) before it is cast again. */
	UPROPERTY(EditAnywhere, Category = "MassWar|FogOfWar", meta = (EditCondition = "bRequireLineOfSight", ClampMin = "0.0"))
	float LineOfSightCacheSeconds = 0.6f;

	/** Width of a lookup grid cell (uu); about a third of a typical sight radius. */
	UPROPERTY(EditAnywhere, Category = "MassWar|FogOfWar", meta = (ClampMin = "100.0"))
	float GridCellSize = 1000.f;

	float TimeSinceLastUpdate = 0.f;

	FMassEntityQuery EntityQuery;

private:
	struct FLineOfSightAnswer
	{
		double Time = 0.0;
		bool bVisible = false;
	};

	/** Last line-of-sight answer per (viewing team, candidate). */
	TMap<uint8, TMap<FMassEntityHandle, FLineOfSightAnswer>> LineOfSightCache;
};
