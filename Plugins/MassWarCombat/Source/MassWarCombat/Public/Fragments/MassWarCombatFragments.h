// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityElementTypes.h"
#include "MassWarCombatFragments.generated.h"

USTRUCT()
struct MASSWARCOMBAT_API FMassWarHealthFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "MassWar|Combat")
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, Category = "MassWar|Combat")
	float Health = 100.f;
};

/**
 * Per-unit attack cooldown state. The numbers that decide how and when a unit attacks (damage, range, interval, noise
 * range) are stats (EMassWarStat::AttackDamage / AttackRange / AttackInterval / AttackNoiseRange), not fields here.
 */
USTRUCT()
struct MASSWARCOMBAT_API FMassWarAttackCooldownFragment : public FMassFragment
{
	GENERATED_BODY()

	/** Seconds accumulated since the last successful attack; processor-owned runtime state. Starts high: ready to fire. */
	UPROPERTY(Transient)
	float TimeSinceLastAttack = 1.e6f;
};
