// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassObserverProcessor.h"
#include "MassEntityQuery.h"
#include "MassWarReplicationEntityDestructor.generated.h"

/**
 * Server-only: UMassReplicationProcessor's own removal path only fires when a still-existing entity's
 * LOD drops to Off (it re-derives the entity's archetype each frame to keep tracking it) - an entity
 * that's outright destroyed (e.g. MassWarCombat killing a unit) just vanishes from every future query,
 * so the engine never gets a chance to notice and tell clients to remove it, leaving a permanently
 * stale proxy on every connected client. This observer reacts to Core's own FMassWarNetIdFragment being
 * removed - which happens automatically whenever any entity carrying it is destroyed, regardless of who
 * destroyed it - and explicitly removes the entity from every client bubble that still has it, closing
 * that gap without MassWarCombat (or anything else) needing to know MassWarReplication exists.
 */
UCLASS()
class MASSWARREPLICATION_API UMassWarReplicationEntityDestructor : public UMassObserverProcessor
{
	GENERATED_BODY()

public:
	UMassWarReplicationEntityDestructor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
