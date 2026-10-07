// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassWarDeathProcessor.generated.h"

/**
 * Finishes off dead units: advances the timer of every Dying entity (FMassWarLifeFragment) and destroys
 * the entity once its LingerTime is up. Whatever killed the unit (MassWarCombat, or anything else later)
 * only flips its state to Dying; this is the single place entities are actually removed because of death.
 * Server/Standalone only - clients see the removal through replication, they never destroy a dying entity
 * on their own (a unit that merely leaves a client's sight is removed the same way, and must not be
 * mistaken for a death).
 */
UCLASS()
class MASSWAR_API UMassWarDeathProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassWarDeathProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
