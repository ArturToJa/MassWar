// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "Stats/MassWarStatTypes.h"
#include "MassWarStatCalculator.generated.h"

class UMassWarStatSubsystem;

/**
 * Read-only view of one set of resolved stat values (a unit's profile, or the values being worked out). Only valid during
 * the call it is handed to - do not store it.
 */
USTRUCT(BlueprintType)
struct MASSWAR_API FMassWarStatReader
{
	GENERATED_BODY()

	const UMassWarStatSubsystem* Subsystem = nullptr;
	const float* Values = nullptr;

	bool IsValid() const { return Subsystem && Values; }

	/** The value of a stat (0 if the reader is empty or the stat unknown). */
	float Get(FMassWarStatId Stat) const;
	float Get(const FGameplayTag& StatTag) const;
};

/**
 * Computes a stat from other stats - the place to say HOW a number is used. Example: attack range that grows with the
 * ammo's muzzle velocity:   AttackRange = BaseRange * (1 + MuzzleVelocity / 1000).
 *
 * Subclass it in C++ or Blueprint and list the class in Project Settings > MassWar Stats > Stat Calculators. Calculators run,
 * in that order, each time a stat profile is worked out (a unit type's, or the one a unit switches to when its weapon, ammo
 * or stat sets change) - once per distinct setup, never per unit and never per frame, so even a Blueprint costs nothing at
 * crowd scale. They see all values after the modifiers have been applied and earlier calculators have run.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew)
class MASSWAR_API UMassWarStatCalculator : public UObject
{
	GENERATED_BODY()

public:
	/** The stat this calculator writes (a built-in such as MassWar.Stat.AttackRange, or one of the project's own). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Calculator")
	FGameplayTag OutputStat;

	/** The new value of OutputStat, from the other values in Stats. The default keeps the value as it is. */
	UFUNCTION(BlueprintNativeEvent, Category = "MassWar|Stats")
	float Calculate(const FMassWarStatReader& Stats) const;
	virtual float Calculate_Implementation(const FMassWarStatReader& Stats) const;
};

/** Blueprint access to a stat reader. */
UCLASS()
class MASSWAR_API UMassWarStatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** A stat's value by tag. */
	UFUNCTION(BlueprintPure, Category = "MassWar|Stats")
	static float GetStat(const FMassWarStatReader& Stats, FGameplayTag StatTag);

	/** A built-in stat's value. */
	UFUNCTION(BlueprintPure, Category = "MassWar|Stats")
	static float GetBuiltInStat(const FMassWarStatReader& Stats, EMassWarStat Stat);
};
