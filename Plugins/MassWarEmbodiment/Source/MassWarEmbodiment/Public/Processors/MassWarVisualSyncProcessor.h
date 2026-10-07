// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassWarVisualSyncProcessor.generated.h"

/**
 * Keeps a near-LOD visual Actor (anything implementing IMassWarVisualPuppet) glued to the Mass entity it
 * represents.
 *
 * Mass's stock representation processor only places a spawned actor when it first appears (or when it
 * switches back from an ISM instance) - nothing moves it afterwards for actors that, like ours, have no
 * UMassAgentComponent. So this runs every frame, after movement, and pushes the entity's transform into
 * the puppet (see IMassWarVisualPuppet::SyncFromEntity). Data only ever flows entity -> actor:
 * the entity stays the single source of truth, which is what lets a unit swap between actor and ISM
 * representations with nothing to transfer or reconcile.
 *
 * Runs on server (listen), standalone and clients alike - each machine drives its own puppets. Entities
 * without the visualization trait (all of them on a dedicated server) never match the query.
 */
UCLASS()
class MASSWAREMBODIMENT_API UMassWarVisualSyncProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassWarVisualSyncProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
