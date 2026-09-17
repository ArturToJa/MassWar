// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityElementTypes.h"
#include "MassEntityHandle.h"
#include "MassWarUnitFragments.generated.h"

/** Which side of a conflict a unit belongs to. Team 0 is reserved for "neutral/unassigned". */
USTRUCT()
struct MASSWAR_API FMassWarTeamFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "MassWar|Team")
	uint8 TeamId = 0;
};

/**
 * Which player controls this unit - independent of team (several players, or players plus bots, can
 * share a team; only the owning player may select/order this specific unit). 0 means "unowned" (e.g. a
 * bot-only unit with no player behind it). Distinct from FMassWarTeamFragment: hostility/combat/fog
 * still only ever consult team, never ownership - two allied players still can't steal each other's
 * units, but they fight and see the map as one side.
 */
USTRUCT()
struct MASSWAR_API FMassWarOwnerFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "MassWar|Owner")
	uint32 OwningPlayerId = 0;
};

UENUM()
enum class EMassWarAffiliation : uint8
{
	Owned,
	Allied,
	Enemy,
	Neutral
};

/** Affiliation of TeamB as seen from TeamA's perspective. Neutral (0) never counts as owned/allied/enemy of anything but itself. */
inline EMassWarAffiliation MassWarGetAffiliation(uint8 TeamA, uint8 TeamB)
{
	if (TeamA == 0 || TeamB == 0)
	{
		return EMassWarAffiliation::Neutral;
	}
	return TeamA == TeamB ? EMassWarAffiliation::Allied : EMassWarAffiliation::Enemy;
}

UENUM()
enum class EMassWarOrderType : uint8
{
	Idle,
	Move,
	Attack
};

/**
 * The single generic "what is this unit currently told to do" fragment. Populated by whatever issues
 * orders (player input, AI, script) and consumed by whatever executes them (movement, combat).
 */
USTRUCT()
struct MASSWAR_API FMassWarOrderFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "MassWar|Order")
	EMassWarOrderType OrderType = EMassWarOrderType::Idle;

	UPROPERTY(EditAnywhere, Category = "MassWar|Order")
	FVector Destination = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "MassWar|Order")
	FMassEntityHandle TargetEntity;

	/**
	 * True while this unit is fulfilling a player-issued order (set by UMassWarUnitOrderComponent).
	 * MassWarStateTreeAI's own auto-engage condition (FMassWarSTCondition_HasEnemyInRange) won't override
	 * the order while this is set, so a player order takes priority over the AI until it naturally
	 * completes - cleared wherever OrderType reverts to Idle on its own (arrival, or the attack target
	 * dying/becoming invalid), at which point normal AI behavior resumes.
	 */
	UPROPERTY(Transient)
	bool bPlayerCommanded = false;
};

/** Movement tuning consumed by MassWarOrderMovementProcessor to turn a Move order into velocity. */
USTRUCT()
struct MASSWAR_API FMassWarMovementParamsFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "MassWar|Movement")
	float MoveSpeed = 500.f;

	/** How close to the destination counts as "arrived" (order reverts to Idle, velocity zeroed). */
	UPROPERTY(EditAnywhere, Category = "MassWar|Movement")
	float AcceptanceRadius = 50.f;
};

/**
 * Stable id that identifies this unit the same way on server and client, independent of either side's
 * local FMassEntityHandle (handles are per-world-instance and not valid across a network connection -
 * a remote client's replicated units live in their own local Mass world with their own handle values).
 * Populated by MassWarReplication (mirrored from the engine's own FMassNetworkIDFragment) when that
 * plugin is present; 0 means "not yet assigned / no networking".
 */
USTRUCT()
struct MASSWAR_API FMassWarNetIdFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "MassWar|Net")
	uint32 NetId = 0;
};
