// Copyright Epic Games, Inc. All Rights Reserved.

#include "Traits/MassWarFarAnimationTrait.h"
#include "MassEntityTemplateRegistry.h"
#include "MassEntityUtils.h"
#include "MassCommonFragments.h"
#include "Engine/World.h"

void UMassWarFarAnimationTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.RequireFragment<FTransformFragment>();
	BuildContext.AddFragment<FMassWarFarAnimationFragment>();

	FMassWarFarAnimationParams Params;
	Params.Clips = Clips;
	Params.SampleRate = FMath::Max(SampleRate, 1.f);
	Params.StartMovingSpeed = StartMovingSpeed;
	Params.StopMovingSpeed = FMath::Min(StopMovingSpeed, StartMovingSpeed);
	Params.RunSpeed = RunSpeed;

	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);
	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(Params));
}
