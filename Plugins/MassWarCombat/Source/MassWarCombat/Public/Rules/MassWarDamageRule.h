// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "Engine/DeveloperSettings.h"
#include "Subsystems/WorldSubsystem.h"
#include "Stats/MassWarStatCalculator.h"
#include "MassWarDamageRule.generated.h"

/**
 * Decides how much damage a landed attack does - the place for armour, damage types, distance fall-off, critical hits...
 * Subclass it in C++ or Blueprint and select the class in Project Settings > MassWar Combat > Damage Rule. Without one the
 * attacker's AttackDamage stat is dealt as it is.
 *
 * Called once per landed attack (a unit attacks about once an interval), not per frame, on the game thread. Both sides'
 * stats are readable (MassWarStatLibrary / FMassWarStatReader), so the rule can compare, say, a project-defined
 * Penetration stat of the attacker with an Armour stat of the target.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew)
class MASSWARCOMBAT_API UMassWarDamageRule : public UObject
{
	GENERATED_BODY()

public:
	/** The damage to deal. BaseDamage is the attacker's AttackDamage stat; Distance is attacker-to-target in uu. */
	UFUNCTION(BlueprintNativeEvent, Category = "MassWar|Combat")
	float ComputeDamage(const FMassWarStatReader& Attacker, const FMassWarStatReader& Target, float BaseDamage, float Distance) const;
	virtual float ComputeDamage_Implementation(const FMassWarStatReader& Attacker, const FMassWarStatReader& Target, float BaseDamage, float Distance) const { return BaseDamage; }
};

/** Project Settings > Plugins > MassWar Combat. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "MassWar Combat"))
class MASSWARCOMBAT_API UMassWarCombatSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMassWarCombatSettings() { CategoryName = TEXT("Plugins"); }

	/** Decides the damage of every landed attack. Empty = the attacker's AttackDamage stat is dealt unchanged. */
	UPROPERTY(EditAnywhere, Config, Category = "Combat")
	TSubclassOf<UMassWarDamageRule> DamageRule;
};

/** Holds the (single) damage rule instance for a world. */
UCLASS()
class MASSWARCOMBAT_API UMassWarCombatSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** The configured rule, null if none. */
	const UMassWarDamageRule* GetDamageRule() const { return DamageRule; }

	/** Replaces the rule from code (tests, mods). */
	void SetDamageRuleOverride(UMassWarDamageRule* InRule) { DamageRule = InRule; }

private:
	UPROPERTY(Transient)
	TObjectPtr<UMassWarDamageRule> DamageRule;
};