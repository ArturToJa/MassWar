// Copyright Epic Games, Inc. All Rights Reserved.

#include "Traits/MassWarCombatTrait.h"
#include "MassEntityTemplateRegistry.h"
#include "Fragments/MassWarCombatFragments.h"

void UMassWarCombatTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	FMassWarHealthFragment& Health = BuildContext.AddFragment_GetRef<FMassWarHealthFragment>();
	Health.MaxHealth = MaxHealth;
	Health.Health = MaxHealth;

	FMassWarCombatParamsFragment& Combat = BuildContext.AddFragment_GetRef<FMassWarCombatParamsFragment>();
	Combat.AttackDamage = AttackDamage;
	Combat.AttackRange = AttackRange;
	Combat.AttackInterval = AttackInterval;
	Combat.TimeSinceLastAttack = AttackInterval;
}
