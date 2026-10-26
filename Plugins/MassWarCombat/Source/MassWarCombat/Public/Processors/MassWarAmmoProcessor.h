// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassWarAmmoProcessor.generated.h"

/**
 * Keeps units' ammunition in order, server-side: fills a weapon's magazine and reserve the first time it is in hand, counts a reload
 * down and completes it (moving rounds from the reserve into the magazine), cancels a reload when another weapon is drawn, and
 * keeps the magazine within a changed MagazineSize. Does nothing for weapons without a magazine (MagazineSize stat 0).
 * The damage processor spends rounds and starts automatic reloads.
 */
UCLASS()
class MASSWARCOMBAT_API UMassWarAmmoProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassWarAmmoProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};