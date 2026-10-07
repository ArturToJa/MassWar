// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassWarUpdateISMProcessor.generated.h"

/**
 * Replaces the engine's UMassUpdateISMProcessor (which this plugin switches off in memory at startup): does exactly what it
 * does - pushes each visible far unit's transform to its instanced mesh - and, for units with the Far Animation
 * trait, decides which baked clip the unit plays and pushes that to the instance as per-instance custom data
 * (FMassWarFarAnimCustomData) in the layout the engine's AnimToTexture layer expects. Mass requires custom data to be pushed for every instance on every update, in the
 * same loop as the transform, which is why the stock processor cannot simply be left alone and supplemented.
 *
 * Clip choice, from the unit's own data so every representation agrees:
 *  - Dying -> a Death clip (once, last frame held)
 *  - a landed attack (Core's attack counter changed) -> an Attack clip once, then back to locomotion
 *  - otherwise Idle, Walk or Run by the speed measured from position changes (velocity is not replicated to
 *    clients; hysteresis stops it flickering). Starting clips are phase-shifted per unit so crowds do not move in lockstep.
 */
UCLASS()
class MASSWAREMBODIMENT_API UMassWarUpdateISMProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassWarUpdateISMProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
