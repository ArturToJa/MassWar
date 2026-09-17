// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassWarOrderMovementProcessor.generated.h"

/**
 * Turns a "Move" order into straight-line velocity toward its destination, reverting the order to Idle
 * once it arrives. Also closes the distance for an "Attack" order whose target is still out of range -
 * MassWarCombat's damage processor only checks range, it never moves anyone, so without this an
 * Attack order on a distant target would leave the unit standing still. Deliberately simple - no
 * pathfinding/obstacle avoidance, that's a documented follow-up (see the plan's "not included" list).
 * Runs on Server/Standalone only; movement is server-authoritative like everything else in MassWar.
 */
UCLASS()
class MASSWAR_API UMassWarOrderMovementProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassWarOrderMovementProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
