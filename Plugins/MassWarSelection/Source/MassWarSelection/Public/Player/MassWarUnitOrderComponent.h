// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "MassEntityHandle.h"
#include "Player/MassWarOrderTarget.h"
#include "MassWarUnitOrderComponent.generated.h"

struct FMassEntityManager;

/**
 * Order dispatch for a player's current selection. Server RPCs carrying FMassWarOrderTarget (handle +
 * NetId) rather than a bare FMassEntityHandle - on a listen server issuing its own local player's orders
 * the Handle resolves directly (same Mass world, no networking involved); once MassWarReplication is
 * installed and a *remote* client issues orders, that client's Handle only means something in its own
 * local replicated Mass world, so the server resolves via NetId instead. See ResolveTarget().
 */
UCLASS(ClassGroup = "MassWar", meta = (BlueprintSpawnableComponent))
class MASSWARSELECTION_API UMassWarUnitOrderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMassWarUnitOrderComponent();

	UFUNCTION(Server, Reliable)
	void ServerIssueMoveOrder(const TArray<FMassWarOrderTarget>& Entities, FVector Destination);

	UFUNCTION(Server, Reliable)
	void ServerIssueAttackOrder(const TArray<FMassWarOrderTarget>& Entities, FMassWarOrderTarget Target);

	/** Move order for whole formations (the normal case: selection is by formation). Only ids travel over the
	 *  network - the server owns the formations and checks that the calling player owns each one. */
	UFUNCTION(Server, Reliable)
	void ServerIssueFormationMoveOrder(const TArray<int32>& FormationIds, FVector Destination);

	/** Attack order for whole formations against an enemy formation (each member is given its own target). */
	UFUNCTION(Server, Reliable)
	void ServerIssueFormationAttackOrder(const TArray<int32>& FormationIds, int32 TargetFormationId);

private:
	/** Trusts Handle only for a local (listen-server host) connection; NetId otherwise - see class comment. Invalid handle if it can't resolve. */
	FMassEntityHandle ResolveTarget(FMassEntityManager& EntityManager, const FMassWarOrderTarget& Target) const;

	/** Authoritative check: does this component's owning player actually own Entity? Client-side selection
	 *  already only lets a player pick their own units, but the server must never trust that - a forged
	 *  RPC could name any entity. */
	bool IsOwnedByCallingPlayer(FMassEntityManager& EntityManager, FMassEntityHandle Entity) const;
};
