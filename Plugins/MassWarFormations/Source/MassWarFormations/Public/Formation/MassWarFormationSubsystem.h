// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "Formation/MassWarFormationTypes.h"
#include "MassWarFormationSubsystem.generated.h"

struct FMassEntityManager;

/**
 * Owns every formation (server only - clients only ever see the replicated FormationId on units). A formation
 * is what gets selected, ordered and (later) pools its members' perception; units are just its members.
 * The formation turns one order into ordinary per-member orders (FMassWarOrderFragment), so movement, pathing,
 * avoidance and combat work unchanged underneath:
 *  - Move: a slot per member around the destination, laid out for the formation's shape and facing the way it
 *    travels. Members walk to their own slot on their own - they arrive as a loose crowd, not in a marching line.
 *  - Attack (an enemy formation): the formation assigns each member a target inside it - nearest first, with a
 *    cap per target so fire spreads - and re-assigns members whose target died.
 */
UCLASS()
class MASSWARFORMATIONS_API UMassWarFormationSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	//~ UTickableWorldSubsystem
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMassWarFormationSubsystem, STATGROUP_Tickables); }

	/** Server: makes a formation of Members (which leave any formation they were in) and returns its id (never 0). */
	uint32 CreateFormation(FMassEntityManager& EntityManager, TConstArrayView<FMassEntityHandle> Members, uint32 OwnerPlayerId, const FMassWarFormationSettings& Settings);

	const FMassWarFormation* FindFormation(uint32 FormationId) const { return Formations.Find(FormationId); }
	const TMap<uint32, FMassWarFormation>& GetFormations() const { return Formations; }

	/** Everything the members of this formation perceive, pooled (empty if no such formation). Server only. */
	TConstArrayView<FMassWarPerceivedEntry> GetKnowledge(uint32 FormationId) const
	{
		const FMassWarFormation* Formation = Formations.Find(FormationId);
		return Formation ? TConstArrayView<FMassWarPerceivedEntry>(Formation->Knowledge) : TConstArrayView<FMassWarPerceivedEntry>();
	}

	/**
	 * Server: Move order for the given formations (those RequesterPlayerId owns). Several formations are placed
	 * side by side around Destination so they don't pile onto each other.
	 */
	void IssueMoveOrder(FMassEntityManager& EntityManager, TConstArrayView<uint32> FormationIds, const FVector& Destination, uint32 RequesterPlayerId);

	/** Server: Attack order against an enemy formation for the given formations (those RequesterPlayerId owns). */
	void IssueAttackOrder(FMassEntityManager& EntityManager, TConstArrayView<uint32> FormationIds, uint32 TargetFormationId, uint32 RequesterPlayerId);

private:
	/** Drops dead members, refreshes the anchor. Returns false if the formation is now empty. */
	bool Refresh(FMassEntityManager& EntityManager, FMassWarFormation& Formation) const;

	void ApplyMove(FMassEntityManager& EntityManager, FMassWarFormation& Formation, const FVector& SlotCentre, const FVector& Forward) const;
	void AssignTargets(FMassEntityManager& EntityManager, FMassWarFormation& Formation, const FMassWarFormation& Target) const;

	/** Merges the members' perception (and what the formation already knew) into Formation.Knowledge. */
	void PoolKnowledge(FMassEntityManager& EntityManager, FMassWarFormation& Formation, double Now) const;

	/** For an idle formation: engage a visible enemy formation, or go and look at a noise / a hit. */
	void AutoBehave(FMassEntityManager& EntityManager, FMassWarFormation& Formation, double Now);

	/** Seconds between formation maintenance passes. */
	static constexpr float MaintenanceInterval = 0.25f;
	/** Sideways gap between formations placed next to each other by one order. */
	static constexpr float FormationGap = 200.f;

	TMap<uint32, FMassWarFormation> Formations;
	uint32 NextFormationId = 1;
	float TimeSinceMaintenance = 0.f;
};
