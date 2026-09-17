// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityTraitBase.h"
#include "MassWarCombatTrait.generated.h"

/** Adds Health + combat params fragments to a MassWar unit. Combine with MassWar (Core)'s
 *  UMassWarUnitTraitBase, which owns the generic Order fragment this trait's processor consumes. */
UCLASS(meta = (DisplayName = "MassWar Combat"))
class MASSWARCOMBAT_API UMassWarCombatTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "MassWar|Combat")
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, Category = "MassWar|Combat")
	float AttackDamage = 10.f;

	UPROPERTY(EditAnywhere, Category = "MassWar|Combat")
	float AttackRange = 800.f;

	UPROPERTY(EditAnywhere, Category = "MassWar|Combat")
	float AttackInterval = 1.f;

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
