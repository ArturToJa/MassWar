// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassWarCustomOrderProcessor.generated.h"

/**
 * Runs the behaviours of units that carry a user-defined order: tells the behaviour when the Move / Attack it handed over has ended
 * (OnPrimitiveFinished, checked every frame - a cheap flag test) and ticks it every TickInterval (OnTick). Ends the order of a unit that
 * died. Game thread, because behaviours may be Blueprints. Server / standalone only.
 */
UCLASS()
class MASSWAR_API UMassWarCustomOrderProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassWarCustomOrderProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};