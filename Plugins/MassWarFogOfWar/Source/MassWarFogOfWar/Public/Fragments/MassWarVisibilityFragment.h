// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityElementTypes.h"
#include "MassWarVisibilityFragment.generated.h"

/** How far this unit can see enemy units, for MassWarVisibilityProcessor's per-team sight computation. */
USTRUCT()
struct MASSWARFOGOFWAR_API FMassWarVisibilityFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "MassWar|FogOfWar")
	float SightRadius = 3000.f;
};
