// Copyright Epic Games, Inc. All Rights Reserved.

#include "Traits/MassWarCombatTrait.h"
#include "MassEntityTemplateRegistry.h"
#include "Fragments/MassWarCombatFragments.h"
#include "Stats/MassWarStatSubsystem.h"

void UMassWarCombatTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	FMassWarHealthFragment& Health = BuildContext.AddFragment_GetRef<FMassWarHealthFragment>();
	Health.MaxHealth = MaxHealth;
	Health.Health = MaxHealth;

	BuildContext.AddFragment<FMassWarAttackCooldownFragment>();

	// Attack numbers are stats; this trait supplies the unit type's base values (see MassWarStats).
	UMassWarStatSubsystem::ContributeBase(BuildContext, World, EMassWarStat::MaxHealth, MaxHealth);
	UMassWarStatSubsystem::ContributeBase(BuildContext, World, EMassWarStat::AttackDamage, AttackDamage);
	UMassWarStatSubsystem::ContributeBase(BuildContext, World, EMassWarStat::AttackRange, AttackRange);
	UMassWarStatSubsystem::ContributeBase(BuildContext, World, EMassWarStat::AttackInterval, AttackInterval);
	UMassWarStatSubsystem::ContributeBase(BuildContext, World, EMassWarStat::AttackNoiseRange, AttackNoiseRange);
}
