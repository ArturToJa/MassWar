// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassWarPerceptionDebugProcessor.generated.h"

/**
 * Draws what units perceive, while the console variable MassWar.Perception.Debug is 1 (does nothing, at
 * essentially no cost, when 0): a line from each perceiving unit to every unit it has perceived -
 * green = in sight now, yellow = heard, red = took damage from, grey = remembered from sight - plus a
 * marker at the last known location. Capped per frame so it stays usable in big crowds.
 */
UCLASS()
class MASSWARPERCEPTION_API UMassWarPerceptionDebugProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassWarPerceptionDebugProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
