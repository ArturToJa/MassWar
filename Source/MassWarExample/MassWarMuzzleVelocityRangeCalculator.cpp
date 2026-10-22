// Copyright Epic Games, Inc. All Rights Reserved.

#include "MassWarMuzzleVelocityRangeCalculator.h"
#include "Stats/MassWarStatTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassWarMuzzleVelocityRangeCalculator)

UMassWarMuzzleVelocityRangeCalculator::UMassWarMuzzleVelocityRangeCalculator()
{
	// Writes the built-in attack range stat.
	OutputStat = FGameplayTag::RequestGameplayTag(TEXT("MassWar.Stat.AttackRange"));
}

float UMassWarMuzzleVelocityRangeCalculator::Calculate_Implementation(const FMassWarStatReader& Stats) const
{
	const float Range = Stats.Get(FMassWarStatId(EMassWarStat::AttackRange));

	// The project's own stat (Project Settings > MassWar Stats). Looked up here, not in the constructor, because project
	// gameplay tags are not loaded yet when class default objects are made.
	const FGameplayTag MuzzleVelocityTag = FGameplayTag::RequestGameplayTag(TEXT("MassWar.Stat.MuzzleVelocity"), false);
	const float MuzzleVelocity = Stats.Get(MuzzleVelocityTag);
	if (MuzzleVelocity <= 0.f)
	{
		return Range;
	}

	return Range * FMath::Pow(MuzzleVelocity / FMath::Max(ReferenceVelocity, 1.f), VelocityExponent);
}
