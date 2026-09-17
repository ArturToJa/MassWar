// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassWarClientInterpolationProcessor.generated.h"

/**
 * Client-only: every tick, smoothly moves FTransformFragment toward FMassWarClientInterpolationFragment's
 * target (set by FMassWarClientBubbleHandler whenever a replication update arrives) instead of the
 * engine's default hard-snap-on-update behavior. See FMassWarClientInterpolationFragment for why.
 */
UCLASS()
class MASSWARREPLICATION_API UMassWarClientInterpolationProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassWarClientInterpolationProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
