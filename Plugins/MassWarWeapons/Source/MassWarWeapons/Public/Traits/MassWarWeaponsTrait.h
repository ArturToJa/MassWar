// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityTraitBase.h"
#include "Weapons/MassWarWeaponSubsystem.h"
#include "MassWarWeaponsTrait.generated.h"

/**
 * Lets a unit type carry weapons: adds its loadout (primary / secondary / special, any may be empty). The default
 * loadout below is what every unit of this type is spawned with; spawn code can give an individual unit different
 * weapons with UMassWarWeaponSubsystem::ApplyLoadout. Weapons must be listed in the weapon catalog.
 */
UCLASS(meta = (DisplayName = "MassWar Weapons"))
class MASSWARWEAPONS_API UMassWarWeaponsTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "MassWar|Weapons")
	FMassWarLoadout DefaultLoadout;

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
