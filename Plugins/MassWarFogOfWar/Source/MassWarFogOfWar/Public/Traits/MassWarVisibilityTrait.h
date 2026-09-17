// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityTraitBase.h"
#include "MassWarVisibilityTrait.generated.h"

/** Adds FMassWarVisibilityFragment so this unit can act as a sight source for its team's fog of war. */
UCLASS(meta = (DisplayName = "MassWar Visibility"))
class MASSWARFOGOFWAR_API UMassWarVisibilityTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "MassWar|FogOfWar")
	float SightRadius = 3000.f;

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
