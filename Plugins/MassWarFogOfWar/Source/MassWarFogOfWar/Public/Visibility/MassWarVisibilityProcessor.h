// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassWarVisibilityProcessor.generated.h"

/**
 * Server-only: every UpdateInterval seconds, for each team, finds which enemy units are within
 * SightRadius of any of that team's own units (radius check only for now - line-of-sight raycasting is
 * a documented follow-up) and publishes the result to UMassWarVisibilitySubsystem. Simulation itself
 * never consults this - every unit's AI/combat/movement keeps running regardless of visibility; this
 * only gates what gets replicated to clients (see UMassWarVisibilitySubsystem::IsRelevant).
 */
UCLASS()
class MASSWARFOGOFWAR_API UMassWarVisibilityProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassWarVisibilityProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	UPROPERTY(EditAnywhere, Category = "MassWar|FogOfWar")
	float UpdateInterval = 0.2f;

	float TimeSinceLastUpdate = 0.f;

	FMassEntityQuery EntityQuery;
};
