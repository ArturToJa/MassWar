// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "MassEntityElementTypes.h"
#include "MassWarStatTypes.generated.h"

/**
 * The stats MassWar's own systems read. They always exist, in this order (the order is the stat's index, so reading one
 * is an array lookup). A project adds its own stats after these in Project Settings > MassWar Stats.
 */
UENUM(BlueprintType)
enum class EMassWarStat : uint8
{
	// Movement
	MoveSpeed,
	TurnRate,
	AcceptanceRadius,
	AttackStopDistance,
	// Avoidance
	AvoidanceRadius,
	AvoidanceStrength,
	// Life
	DeathLingerTime,
	MaxHealth,
	// Combat
	AttackDamage,
	AttackRange,
	AttackInterval,
	AttackNoiseRange,

	Count UMETA(Hidden)
};

/** Hard cap on built-in plus project stats (a profile is a flat array of this many floats at most). */
constexpr int32 MassWarMaxStats = 64;

/** Compact id of a stat: its index in the registry. Cheap to copy and compare; get one from a gameplay tag with
 *  UMassWarStatSubsystem::FindStat, or from a built-in EMassWarStat directly. */
struct FMassWarStatId
{
	static constexpr uint8 InvalidIndex = 0xFF;

	uint8 Index = InvalidIndex;

	FMassWarStatId() = default;
	explicit FMassWarStatId(uint8 InIndex) : Index(InIndex) {}
	FMassWarStatId(EMassWarStat BuiltIn) : Index(static_cast<uint8>(BuiltIn)) {}

	bool IsValid() const { return Index != InvalidIndex; }
	bool operator==(const FMassWarStatId& Other) const { return Index == Other.Index; }
};

/** How a modifier changes a stat. Applied in this order: every Set (in listed order, the last wins), then all Adds
 *  are summed, then all Multiplies are multiplied in:   value = (setValue + sum(Add)) * product(Multiply). */
UENUM(BlueprintType)
enum class EMassWarStatOp : uint8
{
	/** Replace the stat's base value. */
	Set,
	/** Add to the value (negative to subtract). */
	Add,
	/** Multiply the value (1.5 = +50%). */
	Multiply
};

/** One change to one stat. Stat sources (a unit type, a weapon, an upgrade...) are lists of these. */
USTRUCT(BlueprintType)
struct MASSWAR_API FMassWarStatModifier
{
	GENERATED_BODY()

	/** Which stat, by gameplay tag (a built-in such as MassWar.Stat.AttackRange, or one of the project's own). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Stats")
	FGameplayTag Stat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Stats")
	EMassWarStatOp Op = EMassWarStatOp::Add;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Stats")
	float Value = 0.f;
};

/** A project-defined stat (Project Settings > MassWar Stats). */
USTRUCT(BlueprintType)
struct MASSWAR_API FMassWarStatDefinition
{
	GENERATED_BODY()

	/** The stat's name, e.g. MyGame.Stat.MuzzleVelocity. Add the tag in Project Settings > Gameplay Tags first. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MassWar|Stats")
	FGameplayTag Tag;

	/** The value of a unit that no source sets. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MassWar|Stats")
	float DefaultValue = 0.f;
};

/**
 * Which stat profile (a row of resolved stat values, shared by every unit built from the same unit type) a unit uses.
 * Two bytes per unit however many stats there are. Read values through UMassWarStatSubsystem.
 */
USTRUCT()
struct MASSWAR_API FMassWarStatProfileFragment : public FMassFragment
{
	GENERATED_BODY()

	static constexpr uint16 InvalidProfile = 0xFFFF;

	UPROPERTY()
	uint16 ProfileIndex = InvalidProfile;
};

/** How many catalogued stat sets can be applied to one unit at run time. */
constexpr int32 MassWarMaxRuntimeStatSets = 4;

/**
 * The stat sets applied to a unit at run time (upgrades, buffs...), by catalog id (UMassWarStatSettings::StatSetCatalog,
 * id = position + 1, 0 = empty). The unit's stat profile is derived from its unit type's profile plus these sets. This is
 * what is replicated: clients receive the ids and derive the same profile themselves, so the unit's numbers agree on
 * every machine without ever sending the numbers (profile indices are local to each machine and are never sent).
 */
USTRUCT()
struct MASSWAR_API FMassWarStatSetsFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 Ids[4] = { 0, 0, 0, 0 };

	/** The four ids as one number, for replication (0 = none). */
	uint32 Pack() const { return static_cast<uint32>(Ids[0]) | (static_cast<uint32>(Ids[1]) << 8) | (static_cast<uint32>(Ids[2]) << 16) | (static_cast<uint32>(Ids[3]) << 24); }
	void Unpack(uint32 Packed)
	{
		for (int32 Slot = 0; Slot < MassWarMaxRuntimeStatSets; ++Slot)
		{
			Ids[Slot] = static_cast<uint8>((Packed >> (8 * Slot)) & 0xFF);
		}
	}
};