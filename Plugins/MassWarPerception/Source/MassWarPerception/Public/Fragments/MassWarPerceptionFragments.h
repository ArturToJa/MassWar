// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityElementTypes.h"
#include "MassEntityHandle.h"
#include "MassWarPerceptionFragments.generated.h"

UENUM()
enum class EMassWarSense : uint8
{
	Sight,
	Hearing,
	/** The unit was hurt by the source - it learns who and from where, even from outside its vision. */
	Damage
};

/** How many units one unit can keep track of at once. Fixed so the per-unit data never allocates. */
constexpr int32 MassWarPerceptionMaxTracked = 8;

struct FMassWarPerceivedEntry;
/** The entry an AI should care about most from a list: the nearest one in sight; failing that (if bIncludeRemembered)
 *  the most recently perceived one no older than MaxAge. Null if there is none. Works on any list (a unit's own, a formation's pooled). */
inline const FMassWarPerceivedEntry* MassWarFindBestPerceived(TConstArrayView<FMassWarPerceivedEntry> Entries, const FVector& From, double Now, bool bIncludeRemembered, float MaxAge);

/**
 * One perceived unit - the Mass counterpart of an Unreal perception "known actor" with its latest stimulus.
 * Sight is re-evaluated continuously (bSeenNow); hearing and damage are events, so those entries just age.
 */
USTRUCT()
struct MASSWARPERCEPTION_API FMassWarPerceivedEntry
{
	GENERATED_BODY()

	/** The unit that was perceived (the noise maker / attacker / seen unit). */
	FMassEntityHandle Entity;

	/** Where the source was when last perceived (for hearing/damage: where the noise/attack came from). */
	FVector3f LastKnownLocation = FVector3f::ZeroVector;

	/** World time (seconds) of the latest stimulus of any sense. Age = now - this. */
	double LastStimulusTime = 0.0;

	/** 0..1: how clearly it was perceived (sight: closer is stronger; hearing: closer to the noise;
	 *  damage: the amount of damage taken). */
	float Strength = 0.f;

	/** World time this unit last hurt the perceiver / was last heard by it. Kept apart from the latest stimulus
	 *  because sight re-stamps that every scan - "was I hit in the last 3 seconds?" must survive it. */
	double LastDamageTime = -1.0e9;
	double LastHeardTime = -1.0e9;

	/** Which sense produced the latest stimulus. */
	EMassWarSense LastSense = EMassWarSense::Sight;

	/** Line-of-sight bookkeeping (see FMassWarPerceptionParams::bRequireLineOfSight): when it was last ray-tested
	 *  and what the answer was, so the ray is not cast again on every scan. */
	double LastLineOfSightCheck = -1.0e9;
	bool bHadLineOfSight = false;

	/** Set while the unit is in sight right now (as of the last sight update). False = remembered only. */
	bool bSeenNow = false;

	/** Optional tag of the latest stimulus (noise tag, like Unreal's stimulus tag). */
	FName Tag;

	bool IsValid() const { return Entity.IsValid(); }
	double GetAge(double Now) const { return Now - LastStimulusTime; }
	bool WasDamagedBy(double Now, float Window) const { return Now - LastDamageTime <= Window; }
	bool WasHeard(double Now, float Window) const { return Now - LastHeardTime <= Window; }
};

/**
 * What a unit perceives: up to MassWarPerceptionMaxTracked units it has seen, heard or been hurt by, each with
 * its last known location. Written by UMassWarPerceptionProcessor (and damage/noise reports); read by AI.
 * Server-only - perception is simulation state, not replicated.
 */
USTRUCT()
struct MASSWARPERCEPTION_API FMassWarPerceptionFragment : public FMassFragment
{
	GENERATED_BODY()

	FMassWarPerceivedEntry Entries[MassWarPerceptionMaxTracked];
	uint8 Num = 0;

	/** World time of this unit's next sight update; processor-owned (staggers units across frames). */
	double NextUpdateTime = 0.0;

	TConstArrayView<FMassWarPerceivedEntry> GetEntries() const { return MakeArrayView(Entries, Num); }

	FMassWarPerceivedEntry* FindMutable(FMassEntityHandle Entity)
	{
		for (int32 Index = 0; Index < Num; ++Index)
		{
			if (Entries[Index].Entity == Entity)
			{
				return &Entries[Index];
			}
		}
		return nullptr;
	}

	const FMassWarPerceivedEntry* Find(FMassEntityHandle Entity) const
	{
		for (int32 Index = 0; Index < Num; ++Index)
		{
			if (Entries[Index].Entity == Entity)
			{
				return &Entries[Index];
			}
		}
		return nullptr;
	}

	/** Existing entry for Entity, or a fresh one (evicting the stalest remembered-only entry if full).
	 *  Null only if every slot is currently in sight. */
	FMassWarPerceivedEntry* FindOrAdd(FMassEntityHandle Entity, bool* bOutIsNew = nullptr)
	{
		if (bOutIsNew)
		{
			*bOutIsNew = false;
		}
		for (int32 Index = 0; Index < Num; ++Index)
		{
			if (Entries[Index].Entity == Entity)
			{
				return &Entries[Index];
			}
		}
		if (bOutIsNew)
		{
			*bOutIsNew = true;
		}
		int32 Slot;
		if (Num < MassWarPerceptionMaxTracked)
		{
			Slot = Num++;
		}
		else
		{
			Slot = INDEX_NONE;
			for (int32 Index = 0; Index < Num; ++Index)
			{
				if (!Entries[Index].bSeenNow && (Slot == INDEX_NONE || Entries[Index].LastStimulusTime < Entries[Slot].LastStimulusTime))
				{
					Slot = Index;
				}
			}
			if (Slot == INDEX_NONE)
			{
				return nullptr;
			}
		}
		Entries[Slot] = FMassWarPerceivedEntry();
		Entries[Slot].Entity = Entity;
		return &Entries[Slot];
	}

	void RemoveAt(int32 Index)
	{
		Entries[Index] = Entries[Num - 1];
		--Num;
	}

	/**
	 * The unit an AI should care about most: the nearest one currently in sight; failing that (and if
	 * bIncludeRemembered) the most recently perceived one no older than MaxAge. Null if there is none.
	 */
	const FMassWarPerceivedEntry* FindBest(const FVector& From, double Now, bool bIncludeRemembered, float MaxAge) const
	{
		return MassWarFindBestPerceived(GetEntries(), From, Now, bIncludeRemembered, MaxAge);
	}
};

inline const FMassWarPerceivedEntry* MassWarFindBestPerceived(TConstArrayView<FMassWarPerceivedEntry> Entries, const FVector& From, double Now, bool bIncludeRemembered, float MaxAge)
{
	const FMassWarPerceivedEntry* Best = nullptr;
	double BestDistSq = TNumericLimits<double>::Max();
	for (const FMassWarPerceivedEntry& Entry : Entries)
	{
		if (Entry.bSeenNow)
		{
			const double DistSq = FVector::DistSquared(From, FVector(Entry.LastKnownLocation));
			if (!Best || DistSq < BestDistSq)
			{
				Best = &Entry;
				BestDistSq = DistSq;
			}
		}
	}
	if (Best || !bIncludeRemembered)
	{
		return Best;
	}
	for (const FMassWarPerceivedEntry& Entry : Entries)
	{
		if (Entry.GetAge(Now) <= MaxAge && (!Best || Entry.LastStimulusTime > Best->LastStimulusTime))
		{
			Best = &Entry;
		}
	}
	return Best;
}

/** Per-unit-type perception settings; one shared copy for every unit built from the same config. */
USTRUCT()
struct MASSWARPERCEPTION_API FMassWarPerceptionParams : public FMassConstSharedFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Sight")
	bool bCanSee = true;

	/** A unit within this distance (and inside the vision cone) becomes perceived. */
	UPROPERTY(EditAnywhere, Category = "Sight", meta = (EditCondition = "bCanSee", ClampMin = "0.0"))
	float SightRadius = 3000.f;

	/** An already seen unit stays seen until it leaves this (larger) distance - stops flicker at the edge. */
	UPROPERTY(EditAnywhere, Category = "Sight", meta = (EditCondition = "bCanSee", ClampMin = "0.0"))
	float LoseSightRadius = 3300.f;

	/** Half-width of the vision cone in degrees around the unit's facing (180 = sees all around). */
	UPROPERTY(EditAnywhere, Category = "Sight", meta = (EditCondition = "bCanSee", ClampMin = "1.0", ClampMax = "180.0"))
	float SightHalfAngleDegrees = 90.f;

	/** Level geometry blocks sight: a unit inside the cone and radius is only seen if a ray from this unit's eyes
	 *  reaches it. Off = sees through walls. Costs raycasts (shared per-frame budget, MassWar.LOS.RaycastsPerFrame). */
	UPROPERTY(EditAnywhere, Category = "Sight", meta = (EditCondition = "bCanSee"))
	bool bRequireLineOfSight = true;

	/** A line-of-sight answer for a perceived unit is reused for this long (seconds) before it is cast again. */
	UPROPERTY(EditAnywhere, Category = "Sight", meta = (EditCondition = "bCanSee && bRequireLineOfSight", ClampMin = "0.0"))
	float LineOfSightRecheckInterval = 1.f;

	/** Seconds between sight scans of this unit. Higher is cheaper; units are staggered so they don't all scan together. */
	UPROPERTY(EditAnywhere, Category = "Sight", meta = (EditCondition = "bCanSee", ClampMin = "0.05"))
	float SightUpdateInterval = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Hearing")
	bool bCanHear = true;

	/** Scales how far this unit hears: a noise of range R is heard within R * HearingSensitivity. */
	UPROPERTY(EditAnywhere, Category = "Hearing", meta = (EditCondition = "bCanHear", ClampMin = "0.0", ClampMax = "2.0"))
	float HearingSensitivity = 1.f;

	/** How long (seconds) a perceived unit that is no longer sensed is remembered before it is forgotten. */
	UPROPERTY(EditAnywhere, Category = "Memory", meta = (ClampMin = "0.0"))
	float MemoryDuration = 8.f;
};
