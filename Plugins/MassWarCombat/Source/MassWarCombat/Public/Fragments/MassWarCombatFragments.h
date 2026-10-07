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
 * Config (damage/range/interval) and per-entity attack-cooldown state. Kept in one fragment for
 * simplicity in this scaffold rather than splitting config into a shared fragment.
 */
USTRUCT()
struct MASSWARCOMBAT_API FMassWarCombatParamsFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "MassWar|Combat")
	float AttackDamage = 10.f;

	UPROPERTY(EditAnywhere, Category = "MassWar|Combat")
	float AttackRange = 800.f;

	UPROPERTY(EditAnywhere, Category = "MassWar|Combat")
	float AttackInterval = 1.f;

	/** How far (uu) enemies can hear this unit's attacks - reported to MassWarPerception on every landed attack.
	 *  0 = a silent attacker. */
	UPROPERTY(EditAnywhere, Category = "MassWar|Combat")
	float AttackNoiseRange = 2500.f;

	/** Seconds accumulated since the last successful attack; processor-owned runtime state. */
	UPROPERTY(Transient)
	float TimeSinceLastAttack = 0.f;
};
