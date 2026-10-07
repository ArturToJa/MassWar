// Copyright Epic Games, Inc. All Rights Reserved.

#include "Traits/MassWarPerceptionTrait.h"
#include "MassEntityTemplateRegistry.h"
#include "MassEntityUtils.h"
#include "MassCommonFragments.h"
#include "Fragments/MassWarUnitFragments.h"
#include "Engine/World.h"

void UMassWarPerceptionTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.RequireFragment<FTransformFragment>();
	BuildContext.RequireFragment<FMassWarTeamFragment>();
	BuildContext.RequireFragment<FMassWarLifeFragment>();

	BuildContext.AddFragment<FMassWarPerceptionFragment>();

	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);
	FMassWarPerceptionParams Validated = Params;
	Validated.LoseSightRadius = FMath::Max(Validated.LoseSightRadius, Validated.SightRadius);
	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(Validated));
}
