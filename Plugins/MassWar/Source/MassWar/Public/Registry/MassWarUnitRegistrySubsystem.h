// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "MassEntityHandle.h"
#include "MassWarUnitHandle.h"
#include "MassWarUnitRegistrySubsystem.generated.h"

struct FMassEntityManager;

/**
 * Lightweight registry of every MassWar unit spawned in this world. Exists so gameplay code that isn't a
 * Mass processor (selection hit-testing, debug tooling, AI target-finding) has a way to enumerate units
 * without needing an ad-hoc FMassEntityQuery. Entries aren't automatically pruned when a unit dies -
 * callers should check validity themselves (FMassEntityManager::IsEntityValid).
 */
UCLASS()
class MASSWAR_API UMassWarUnitRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterUnit(FMassEntityHandle Entity);
	void UnregisterUnit(FMassEntityHandle Entity);

	const TArray<FMassEntityHandle>& GetAllUnits() const { return Units; }

	/** Every known unit as a FMassWarUnitHandle (the form StateTree nodes and FMassWarUnitStateView use). */
	TArray<FMassWarUnitHandle> GetAllUnitHandles() const;

	/**
	 * Resolves a unit by its stable FMassWarNetIdFragment::NetId rather than by FMassEntityHandle - the
	 * only lookup that's meaningful when the caller's handle came from a different Mass world (e.g. a
	 * server RPC parameter sent by a remote client, whose own local handle means nothing here). Linear
	 * scan; fine at MassWar's current unit counts and called only from order-issuing, not per-tick.
	 * Returns an invalid handle if NetId is 0 (unassigned) or not found (e.g. MassWarReplication isn't
	 * installed, so no unit ever gets a non-zero NetId).
	 */
	FMassEntityHandle FindByNetId(const FMassEntityManager& EntityManager, uint32 NetId) const;

private:
	UPROPERTY()
	TArray<FMassEntityHandle> Units;
};
