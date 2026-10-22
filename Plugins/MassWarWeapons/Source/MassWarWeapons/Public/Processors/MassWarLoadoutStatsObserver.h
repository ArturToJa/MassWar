// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassObserverProcessor.h"
#include "MassEntityQuery.h"
#include "MassWarLoadoutStatsObserver.generated.h"

/**
 * Gives a freshly created unit the stats of the weapon and ammo its unit type carries by default (the MassWar Weapons trait's
 * default loadout), on server and client alike. Units that get another loadout afterwards (spawn-time loadout, replication)
 * are refreshed by whatever changes it.
 */
UCLASS()
class MASSWARWEAPONS_API UMassWarLoadoutStatsObserver : public UMassObserverProcessor
{
	GENERATED_BODY()

public:
	UMassWarLoadoutStatsObserver();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};