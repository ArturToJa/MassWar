// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassWarDamageProcessor.generated.h"

/**
 * Server-only (Standalone/Server net modes): resolves any entity whose Order is currently "Attack" into
 * periodic damage against its target, while the target stays alive and in range. Doesn't care who or
 * what issued the order (player RPC, AI StateTree task, a debug command) - it just executes it.
 *
 * Known scaffold simplification: mutates the target's health via direct random-access fragment write
 * rather than a deferred damage-event queue, so two attackers hitting the same target in the same tick
 * from different chunks are not fully thread-safe if this processor is ever parallelized across chunks.
 * Fine for now; a signal/event-queue based version is a natural follow-up if that becomes a problem.
 */
UCLASS()
class MASSWARCOMBAT_API UMassWarDamageProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassWarDamageProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;

private:
	/** Bound to UMassWarUnitRegistrySubsystem::OnDealDamage - lets an optional plugin with no combat math
	 *  of its own (MassWarEmbodiment's hero) still damage a Mass entity target. */
	bool HandleDealDamage(FMassEntityHandle Target, float Damage, FMassEntityHandle Instigator);

	TSharedPtr<FMassEntityManager> CachedEntityManager;
};
