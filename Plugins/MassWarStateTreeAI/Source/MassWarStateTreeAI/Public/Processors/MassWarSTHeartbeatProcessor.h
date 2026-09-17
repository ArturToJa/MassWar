// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassWarSTHeartbeatProcessor.generated.h"

/**
 * Mass's StateTree processor is signal-driven: an entity's tree is only re-ticked when a specific
 * named signal fires for it (StateTreeActivate on start, or event signals like HitReceived /
 * DelayedTransitionWakeup / NewStateTreeTaskRequired). Entering a new Running state does not, by
 * itself, schedule any further re-evaluation. Our tree relies on OnTick transition conditions and
 * on tasks noticing external state changes (e.g. an order fragment flipped to Idle by a separate
 * Mass movement processor), so without a periodic nudge it ticks exactly once and then parks
 * forever. This processor re-signals every entity running a MassWar StateTree at a fixed cadence
 * so those checks actually get re-evaluated.
 */
UCLASS()
class MASSWARSTATETREEAI_API UMassWarSTHeartbeatProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassWarSTHeartbeatProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;

	/** How often (in seconds) to re-signal every StateTree-driven entity. */
	UPROPERTY(EditDefaultsOnly, Category = "MassWar", config)
	float SignalIntervalSeconds = 0.2f;

	float TimeSinceLastSignal = 0.f;
};
