// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassWarDyingVisibilityProcessor.generated.h"

/**
 * Hides a dying unit unless it is close enough to be shown as an Actor puppet (High LOD).
 *
 * A dead unit lingers as a Dying entity so its puppet can play a death animation. Only a puppet can do
 * that - a far unit drawn as an ISM cube would just freeze in place for the whole linger time. This
 * forces such a unit's LOD to Off after the LOD calculation and before Mass chooses a representation, so
 * the cube (or a puppet that is only Medium/Low LOD) disappears immediately, exactly as before dying
 * existed. Purely cosmetic and local to each machine's own camera; the entity itself is untouched.
 * Entities without visualization (all of them on a dedicated server) never match the query.
 */
UCLASS()
class MASSWAREMBODIMENT_API UMassWarDyingVisibilityProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassWarDyingVisibilityProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
