// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityTraitBase.h"
#include "Fragments/MassWarPerceptionFragments.h"
#include "MassWarPerceptionTrait.generated.h"

/** Lets a MassWar unit perceive: sight, hearing and damage, remembered for a while. Combine with MassWar (Core)'s
 *  unit trait, which owns the team and transform this trait's processor reads. */
UCLASS(meta = (DisplayName = "MassWar Perception"))
class MASSWARPERCEPTION_API UMassWarPerceptionTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "MassWar|Perception", meta = (ShowOnlyInnerProperties))
	FMassWarPerceptionParams Params;

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
