// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityElementTypes.h"
#include "MassEntityHandle.h"
#include "MassExternalSubsystemTraits.h"
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

	/** Move orders only. False (a player's click order): the order completes when the unit is really at
	 *  Destination (the AcceptanceRadius stat). True (an AI approach-to-engage move,
	 *  e.g. the StateTree's Move To Range): it completes once within AttackStopDistance of Destination -
	 *  the unit only needs to get into fighting range, not onto the enemy. */
	UPROPERTY(Transient)
	bool bStopAtAttackDistance = false;

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

UENUM()
enum class EMassWarLifeState : uint8
{
	Alive,
	/** Dead but not yet removed: the entity lingers for FMassWarLifeFragment::LingerTime so its visual can
	 *  play a death animation. A dying unit takes no part in gameplay (can't be targeted, ordered, or act). */
	Dying
};

/**
 * Whether a unit is alive, and how long a dead one stays around before its entity is destroyed. Anything
 * that kills a unit (MassWarCombat's damage processor today) sets State to Dying instead of destroying
 * the entity; UMassWarDeathProcessor destroys it once LingerTime has passed. State is replicated to
 * clients (MassWarReplication) so their visuals can animate the death - clients never destroy a dying
 * entity themselves, they only see the server's removal afterwards.
 */
USTRUCT()
struct MASSWAR_API FMassWarLifeFragment : public FMassFragment
{
	GENERATED_BODY()

	bool IsDying() const { return State == EMassWarLifeState::Dying; }

	UPROPERTY(VisibleAnywhere, Category = "MassWar|Life")
	EMassWarLifeState State = EMassWarLifeState::Alive;

	/** Seconds a dead unit stays as a Dying entity before it is destroyed (copied from the unit trait). */
	UPROPERTY(EditAnywhere, Category = "MassWar|Life")
	float LingerTime = 3.f;

	/** Seconds spent Dying so far; server-side runtime state. */
	UPROPERTY(Transient)
	float TimeDying = 0.f;
};

/**
 * Cosmetic feedback: AttackCounter increments (and wraps) every time this unit lands an attack. It is an
 * event counter, not a state - a client's puppet compares it with the last value it saw and plays an attack
 * animation on each change. Unlike an "is attacking" flag it can't be missed when animation updates are
 * throttled for crowds, and it needs no timing on the client. Replicated to clients by MassWarReplication;
 * whatever makes a unit attack (MassWarCombat today) bumps it.
 */
USTRUCT()
struct MASSWAR_API FMassWarAttackFeedbackFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "MassWar|Combat")
	uint8 AttackCounter = 0;
};

UENUM()
enum class EMassWarNavPathState : uint8
{
	/** No path for the current order yet (or none needed - the unit is idle). Move in a straight line. */
	None,
	/** Waypoints hold a path computed for PathDestination. */
	Ready,
	/** No navmesh, or no route - move in a straight line rather than not at all. */
	Failed
};

/**
 * Opt-in (FMassWarUnitTraitBase::bUseNavMesh): lets a unit route around static obstacles using the level's
 * navigation mesh. Holds the path UMassWarNavPathProcessor computed for the unit's current order and that
 * UMassWarOrderMovementProcessor steers along. Units without this fragment simply move in a straight line.
 * Server-side state only - clients get positions, never paths.
 */
USTRUCT()
struct MASSWAR_API FMassWarNavPathFragment : public FMassFragment
{
	GENERATED_BODY()

	/** Corner points from the unit's position when the path was computed to PathDestination (or the nearest
	 *  reachable point, for a partial path); the start point itself is not included. */
	UPROPERTY(Transient)
	TArray<FVector> Waypoints;

	/** Index into Waypoints of the corner being steered toward. */
	UPROPERTY(Transient)
	int32 NextWaypoint = 0;

	/** Where the path leads, and which attack target it was for - a path is only followed while these still
	 *  match the unit's order. */
	UPROPERTY(Transient)
	FVector PathDestination = FVector::ZeroVector;

	UPROPERTY(Transient)
	FMassEntityHandle PathTargetEntity;

	UPROPERTY(Transient)
	EMassWarNavPathState State = EMassWarNavPathState::None;

	/** Seconds since this unit last asked for a path. */
	UPROPERTY(Transient)
	float TimeSinceRequest = 0.f;

	/** Tuning, copied from the unit trait. Minimum seconds between path requests for a chased (moving) goal. */
	UPROPERTY(EditAnywhere, Category = "MassWar|Navigation")
	float RepathInterval = 1.f;

	/** Tuning, copied from the unit trait. How far a chased goal must move before the path is recomputed. */
	UPROPERTY(EditAnywhere, Category = "MassWar|Navigation")
	float RepathDistance = 300.f;
};

/** Holds a TArray, so it is not trivially copyable - Mass requires the fragment's author to say that's intended. */
template<>
struct TMassFragmentTraits<FMassWarNavPathFragment> final
{
	enum
	{
		AuthorAcceptsItsNotTriviallyCopyable = true
	};
};

/**
 * Opts a unit into unit-vs-unit separation (MassWarSeparationProcessor); its radius and strength come from its stats. Units without it neither push nor
 * are pushed - they pass through everyone.
 */
USTRUCT()
struct MASSWAR_API FMassWarAvoidanceFragment : public FMassFragment
{
	GENERATED_BODY()

	// Radius and strength are stats (EMassWarStat::AvoidanceRadius / AvoidanceStrength).
};

/**
 * Stable id that identifies this unit the same way on server and client, independent of either side's
 * local FMassEntityHandle (handles are per-world-instance and not valid across a network connection -
 * a remote client's replicated units live in their own local Mass world with their own handle values).
 * Populated by MassWarReplication (mirrored from the engine's own FMassNetworkIDFragment) when that
 * plugin is present; 0 means "not yet assigned / no networking".
 */
/**
 * Which formation this unit belongs to; 0 = none. A formation is the unit of selection, orders and shared
 * knowledge (see MassWarFormations, which owns the formation records) - Core only carries the id so Selection
 * and Replication can group units without depending on it. Replicated: clients group selectable units by it.
 */
USTRUCT()
struct MASSWAR_API FMassWarFormationMemberFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "MassWar|Formation")
	uint32 FormationId = 0;
};

/** The three places a unit can carry a weapon. Any of them may be empty. */
UENUM(BlueprintType)
enum class EMassWarWeaponSlot : uint8
{
	/** Main weapon - e.g. an assault rifle, carbine or sniper rifle. */
	Primary,
	/** Sidearm - e.g. a pistol. */
	Secondary,
	/** Special weapon - e.g. a rocket launcher. */
	Special
};

constexpr int32 MassWarWeaponSlotCount = 3;

/**
 * What a unit carries: one weapon id per slot (0 = empty; otherwise the weapon's index in the project's weapon
 * catalog + 1, see MassWarWeapons) and which slot is in hand. Core only holds the compact ids so that Replication
 * can send them without depending on the weapons plugin (same idea as FormationId). Added by MassWarWeapons' trait.
 */
USTRUCT()
struct MASSWAR_API FMassWarLoadoutFragment : public FMassFragment
{
	GENERATED_BODY()

	static constexpr uint8 NoActiveSlot = 0xFF;

	/** Reflected on purpose: Mass copies a template's initial fragment values by reflection, so a plain member would be lost. */
	UPROPERTY(VisibleAnywhere, Category = "MassWar|Weapons")
	uint8 WeaponIds[MassWarWeaponSlotCount] = { 0, 0, 0 };
	UPROPERTY(VisibleAnywhere, Category = "MassWar|Weapons")
	uint8 ActiveSlot = NoActiveSlot;

	/** The ammo loaded in each slot's weapon (ammo catalog id, 0 = none). Its stat set is part of the unit's stats while that weapon is in hand. */
	UPROPERTY(VisibleAnywhere, Category = "MassWar|Weapons")
	uint8 AmmoIds[MassWarWeaponSlotCount] = { 0, 0, 0 };

	uint8 GetActiveWeaponId() const { return ActiveSlot < MassWarWeaponSlotCount ? WeaponIds[ActiveSlot] : 0; }
	uint8 GetActiveAmmoId() const { return ActiveSlot < MassWarWeaponSlotCount ? AmmoIds[ActiveSlot] : 0; }

	/** The three ammo ids in one value, for replication. */
	uint32 PackAmmo() const { return static_cast<uint32>(AmmoIds[0]) | (static_cast<uint32>(AmmoIds[1]) << 8) | (static_cast<uint32>(AmmoIds[2]) << 16); }
	void UnpackAmmo(uint32 Packed)
	{
		AmmoIds[0] = static_cast<uint8>(Packed);
		AmmoIds[1] = static_cast<uint8>(Packed >> 8);
		AmmoIds[2] = static_cast<uint8>(Packed >> 16);
	}

	/** All four bytes in one value, for replication. */
	uint32 Pack() const { return static_cast<uint32>(WeaponIds[0]) | (static_cast<uint32>(WeaponIds[1]) << 8) | (static_cast<uint32>(WeaponIds[2]) << 16) | (static_cast<uint32>(ActiveSlot) << 24); }

	void Unpack(uint32 Packed)
	{
		WeaponIds[0] = static_cast<uint8>(Packed);
		WeaponIds[1] = static_cast<uint8>(Packed >> 8);
		WeaponIds[2] = static_cast<uint8>(Packed >> 16);
		ActiveSlot = static_cast<uint8>(Packed >> 24);
	}
};

/**
 * How many rounds a unit's weapons have loaded and in reserve, and whether it is reloading. Only meaningful for a weapon whose
 * MagazineSize stat is above 0 (a weapon with 0 has no ammo limit and this fragment is left alone). The numbers that drive it -
 * magazine size, reserve, reload time, rounds per attack, auto reload - are stats, so weapons, ammo and upgrades set them.
 * A slot starts "not initialised" and is filled (full magazine, full reserve) the first time its weapon is in hand.
 * Server-owned; the active slot's numbers and the reloading flag are replicated for HUDs and reload animations.
 */
USTRUCT()
struct MASSWAR_API FMassWarAmmoFragment : public FMassFragment
{
	GENERATED_BODY()

	static constexpr uint16 NotInitialised = 0xFFFF;
	static constexpr uint8 NotReloading = 0xFF;

	/** Rounds in the magazine of each slot's weapon. */
	UPROPERTY()
	uint16 Loaded[MassWarWeaponSlotCount] = { NotInitialised, NotInitialised, NotInitialised };

	/** Rounds in reserve for each slot's weapon (ignored while that weapon's ReserveAmmo stat is negative = unlimited). */
	UPROPERTY()
	uint16 Reserve[MassWarWeaponSlotCount] = { 0, 0, 0 };

	/** Seconds left of the reload in progress. */
	UPROPERTY(Transient)
	float ReloadRemaining = 0.f;

	/** The slot being reloaded, NotReloading if none. */
	UPROPERTY(Transient)
	uint8 ReloadingSlot = NotReloading;

	bool IsReloading() const { return ReloadingSlot != NotReloading; }

	/** Loaded / reserve / reloading of one slot in one value, for replication (Loaded 13 bits, Reserve 18 bits, reloading 1 bit). */
	uint32 Pack(uint8 Slot) const
	{
		if (Slot >= MassWarWeaponSlotCount)
		{
			return 0;
		}
		const uint32 PackedLoaded = Loaded[Slot] == NotInitialised ? 0x1FFFu : FMath::Min<uint32>(Loaded[Slot], 0x1FFEu);
		const uint32 PackedReserve = FMath::Min<uint32>(Reserve[Slot], 0x3FFFFu);
		return PackedLoaded | (PackedReserve << 13) | (ReloadingSlot == Slot ? 0x80000000u : 0u);
	}

	void Unpack(uint32 Packed, uint8 Slot)
	{
		if (Slot >= MassWarWeaponSlotCount)
		{
			return;
		}
		const uint32 PackedLoaded = Packed & 0x1FFFu;
		Loaded[Slot] = PackedLoaded == 0x1FFFu ? NotInitialised : static_cast<uint16>(PackedLoaded);
		Reserve[Slot] = static_cast<uint16>((Packed >> 13) & 0x3FFFFu);
		const bool bReloading = (Packed & 0x80000000u) != 0;
		if (bReloading)
		{
			ReloadingSlot = Slot;
			ReloadRemaining = FMath::Max(ReloadRemaining, 0.01f);
		}
		else if (ReloadingSlot == Slot)
		{
			ReloadingSlot = NotReloading;
			ReloadRemaining = 0.f;
		}
	}
};
USTRUCT()
struct MASSWAR_API FMassWarNetIdFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "MassWar|Net")
	uint32 NetId = 0;
};
