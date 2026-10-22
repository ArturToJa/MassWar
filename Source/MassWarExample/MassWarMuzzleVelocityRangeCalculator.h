// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Stats/MassWarStatCalculator.h"
#include "MassWarMuzzleVelocityRangeCalculator.generated.h"

/**
 * The example project's rule for how muzzle velocity affects range: a unit's attack range (after its weapon's and ammo's stat
 * sets were applied) is scaled by how much faster than ReferenceVelocity its ammo leaves the barrel,
 *
 *     AttackRange = AttackRange * (MuzzleVelocity / ReferenceVelocity) ^ VelocityExponent
 *
 * A unit whose MuzzleVelocity is 0 (melee, or ammo that does not define one) keeps its range as it is.
 *
 * To use it, add this class to Project Settings > MassWar Stats > Stat Calculators. It runs once for each distinct stat setup
 * (unit type + weapon + ammo + stat sets), never per unit or per frame.
 */
UCLASS(meta = (DisplayName = "Muzzle Velocity -> Attack Range"))
class MASSWAREXAMPLE_API UMassWarMuzzleVelocityRangeCalculator : public UMassWarStatCalculator
{
	GENERATED_BODY()

public:
	UMassWarMuzzleVelocityRangeCalculator();

	/** The muzzle velocity at which range is left as it is. Ammo faster than this gives more range, slower gives less. */
	UPROPERTY(EditDefaultsOnly, Category = "Calculator", meta = (ClampMin = "1.0"))
	float ReferenceVelocity = 800.f;

	/** 1 = range grows in proportion to velocity; 2 = in proportion to its square (the physical maximum range of a projectile). */
	UPROPERTY(EditDefaultsOnly, Category = "Calculator", meta = (ClampMin = "0.1", ClampMax = "4.0"))
	float VelocityExponent = 1.f;

	virtual float Calculate_Implementation(const FMassWarStatReader& Stats) const override;
};
