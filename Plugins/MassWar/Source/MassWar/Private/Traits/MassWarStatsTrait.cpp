// Copyright Epic Games, Inc. All Rights Reserved.

#include "Traits/MassWarStatsTrait.h"
#include "Stats/MassWarStatSubsystem.h"
#include "Stats/MassWarStatSet.h"
#include "MassEntityTemplateRegistry.h"

void UMassWarStatsTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	for (const FMassWarStatModifier& Modifier : Modifiers)
	{
		UMassWarStatSubsystem::ContributeModifier(BuildContext, World, Modifier);
	}
	for (const UMassWarStatSet* StatSet : StatSets)
	{
		UMassWarStatSubsystem::ContributeStatSet(BuildContext, World, StatSet);
	}
}
