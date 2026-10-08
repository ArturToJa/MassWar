// Copyright Epic Games, Inc. All Rights Reserved.

#include "Traits/MassWarWeaponsTrait.h"
#include "MassEntityTemplateRegistry.h"
#include "Engine/World.h"

void UMassWarWeaponsTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	FMassWarLoadoutFragment& Loadout = BuildContext.AddFragment_GetRef<FMassWarLoadoutFragment>();
	if (const UMassWarWeaponSubsystem* Weapons = UWorld::GetSubsystem<UMassWarWeaponSubsystem>(&World))
	{
		Loadout = Weapons->MakeFragment(DefaultLoadout);
	}
}
