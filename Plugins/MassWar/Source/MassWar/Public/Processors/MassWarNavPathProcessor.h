// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassWarNavPathProcessor.generated.h"

/**
 * Computes navmesh paths for units that opted in (FMassWarNavPathFragment / bUseNavMesh) and stores them for
 * UMassWarOrderMovementProcessor to steer along. Keeps pathfinding cheap enough for large armies:
 *  - A unit only asks for a path when its order needs one: a new Move destination, a new attack target, or a
 *    chased target that has moved far enough (and it has been long enough since the last request).
 *  - Most goals on open ground are reachable in a straight line, which a navmesh raycast confirms far more
 *    cheaply than pathfinding - only blocked lines pay for a real path query.
 *  - At most MaxPathRequestsPerFrame units are served per frame; the rest simply wait for the next frame
 *    (meanwhile they keep moving straight, as units without navigation do).
 * Server/Standalone only; a unit's path is server-side state and never replicated. Runs on the game thread
 * because navigation queries are game-thread API.
 */
UCLASS()
class MASSWAR_API UMassWarNavPathProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassWarNavPathProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;

	/** Cap on units given a path per frame - the main knob for the per-frame navigation cost. */
	UPROPERTY(EditAnywhere, config, Category = "MassWar|Navigation", meta = (ClampMin = "1"))
	int32 MaxPathRequestsPerFrame = 24;

	/** A path older than this (seconds) is recomputed even if nothing about the order changed, so a unit never
	 *  keeps following a route the world has moved on from (a Move order's destination never changes). */
	UPROPERTY(EditAnywhere, config, Category = "MassWar|Navigation", meta = (ClampMin = "1.0"))
	float MaxPathAgeSeconds = 6.f;
};
