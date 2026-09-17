// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "MassEntityHandle.h"
#include "MassWarUnitHandle.h"
#include "MassWarUnitRegistrySubsystem.generated.h"

struct FMassEntityManager;

/**
 * Optional "please deal damage" hook, exposed by Core so an optional plugin with no combat math of its
 * own (MassWarEmbodiment's hero, which doesn't depend on MassWarCombat) can still damage a Mass entity
 * target - MassWarCombat's damage processor binds this once per world, Core never knows MassWarCombat
 * exists. Same "expose here, bind there" shape as MassWarReplication's OnFilterRelevancy. Returns true
 * if damage was actually applied (false if MassWarCombat isn't installed, or Target has no health).
 */
DECLARE_DELEGATE_RetVal_ThreeParams(bool, FMassWarDealDamageDelegate, FMassEntityHandle /*Target*/, float /*Damage*/, FMassEntityHandle /*Instigator*/);

/**
 * Lightweight registry of every MassWar unit spawned in this world - both ordinary Mass entities and
 * (MassWarEmbodiment) standalone Actor units. Exists so gameplay code that isn't a Mass processor
 * (selection hit-testing, debug tooling, AI target-finding) has a way to enumerate units without
 * needing an ad-hoc FMassEntityQuery. Entries aren't automatically pruned when a unit dies - callers
 * should check validity themselves (FMassEntityManager::IsEntityValid / TWeakObjectPtr::IsValid).
 */
UCLASS()
class MASSWAR_API UMassWarUnitRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterUnit(FMassEntityHandle Entity);
	void UnregisterUnit(FMassEntityHandle Entity);

	/** MassWarEmbodiment-only in practice, but Core doesn't need to know that to host the list. */
	void RegisterActorUnit(AActor* Actor);
	void UnregisterActorUnit(AActor* Actor);

	const TArray<FMassEntityHandle>& GetAllUnits() const { return Units; }

	/** Every known unit, Mass entities and Actor units combined - see FMassWarUnitHandle. */
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

	/** Bind via GetWorld()->GetSubsystem<UMassWarUnitRegistrySubsystem>()->OnDealDamage.BindUObject(...). */
	FMassWarDealDamageDelegate OnDealDamage;

private:
	UPROPERTY()
	TArray<FMassEntityHandle> Units;

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> ActorUnits;
};
