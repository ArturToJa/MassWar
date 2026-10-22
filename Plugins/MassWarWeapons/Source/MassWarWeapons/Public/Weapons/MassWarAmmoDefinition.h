// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "MassWarAmmoDefinition.generated.h"

class UMassWarStatSet;

/**
 * One kind of ammunition, authored as a data asset in the game project (the plugin ships none) and listed in the project's
 * ammo catalog. A weapon names the ammo it accepts; a unit carries one ammo per weapon, and while that weapon is in hand the
 * ammo's stat set is part of the unit's stats. That is how "this bullet has a higher muzzle velocity" is expressed: define a
 * stat for it (Project Settings > MassWar Stats), put its value in this ammo's stat set, and let a stat calculator turn
 * velocity into range (or damage, or whatever you decide).
 */
UCLASS(BlueprintType)
class MASSWARWEAPONS_API UMassWarAmmoDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Shown in tools and logs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo")
	FText DisplayName;

	/** Changes to the unit's stats while it fires this ammo (e.g. +muzzle velocity, -damage...). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo")
	TObjectPtr<UMassWarStatSet> StatSet;
};