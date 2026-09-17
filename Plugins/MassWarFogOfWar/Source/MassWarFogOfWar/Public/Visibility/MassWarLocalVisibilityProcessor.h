// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassWarLocalVisibilityProcessor.generated.h"

/**
 * MassWarVisibilitySubsystem's team-filtered replication (see UMassWarReplicator/OnFilterRelevancy)
 * only ever affects what gets sent to a *remote* client - a listen server (or a standalone game) never
 * replicates to itself, so without this, the server's own local screen would render every unit
 * regardless of team, fog or no fog. This processor masks the *locally rendered* view the same way:
 * every tick, it finds this process's own local player (there is at most one - a dedicated server has
 * none, so this is a no-op there) and forces FMassRepresentationLODFragment.LOD to Off for any
 * enemy-team entity that team can't currently see, right before UMassVisualizationProcessor reads that
 * LOD to decide whether to actually render the entity. Runs after the LOD group (so it overrides the
 * normal distance-based LOD) and before the Representation group (so the override is what actually gets
 * rendered this frame).
 */
UCLASS()
class MASSWARFOGOFWAR_API UMassWarLocalVisibilityProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassWarLocalVisibilityProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
