// Copyright Epic Games, Inc. All Rights Reserved.

#include "Traits/MassWarWeaponsTrait.h"
#include "MassEntityTemplateRegistry.h"
#include "Engine/World.h"

void UMassWarWeaponsTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	// Ammunition state (rounds loaded / in reserve, reloading); only used by weapons whose MagazineSize stat is above 0.
	BuildContext.AddFragment<FMassWarAmmoFragment>();

	FMassWarLoadoutFragment& Loadout = BuildContext.AddFragment_GetRef<FMassWarLoadoutFragment>();
	if (const UMassWarWeaponSubsystem* Weapons = UWorld::GetSubsystem<UMassWarWeaponSubsystem>(&World))
	{
		Loadout = Weapons->MakeFragment(DefaultLoadout);
	}
}
