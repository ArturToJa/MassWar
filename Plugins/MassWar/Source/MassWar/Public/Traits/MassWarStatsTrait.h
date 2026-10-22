// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityTraitBase.h"
#include "Stats/MassWarStatTypes.h"
#include "MassWarStatsTrait.generated.h"

class UMassWarStatSet;

/**
 * Changes the stats of a unit type on top of what MassWar Unit and MassWar Combat set from their properties: give a
 * project stat (MyGame.Stat.MuzzleVelocity) a value, or scale a built-in one ("this unit type is 20% faster").
 * Optional - a unit type without it simply has the values its other traits set and the stat defaults for the rest.
 * The order of the traits on the config does not matter.
 */
UCLASS(meta = (DisplayName = "MassWar Stats"))
class MASSWAR_API UMassWarStatsTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

public:
	/** Changes applied to this unit type's stats, in this order within each kind (all Sets, then Adds, then Multiplies - see EMassWarStatOp). */
	UPROPERTY(EditAnywhere, Category = "MassWar|Stats", meta = (TitleProperty = "Stat"))
	TArray<FMassWarStatModifier> Modifiers;

	/** Reusable lists of changes (data assets), applied with the ones above. */
	UPROPERTY(EditAnywhere, Category = "MassWar|Stats")
	TArray<TObjectPtr<UMassWarStatSet>> StatSets;

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
