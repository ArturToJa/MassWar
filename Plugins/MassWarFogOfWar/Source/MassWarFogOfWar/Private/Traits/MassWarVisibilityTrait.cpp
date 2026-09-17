// Copyright Epic Games, Inc. All Rights Reserved.

#include "Traits/MassWarVisibilityTrait.h"
#include "MassEntityTemplateRegistry.h"
#include "Fragments/MassWarVisibilityFragment.h"

void UMassWarVisibilityTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.AddFragment_GetRef<FMassWarVisibilityFragment>().SightRadius = SightRadius;
}
