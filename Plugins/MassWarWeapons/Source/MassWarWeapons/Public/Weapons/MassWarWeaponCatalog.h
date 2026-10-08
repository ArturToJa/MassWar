// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "Engine/DeveloperSettings.h"
#include "MassWarWeaponCatalog.generated.h"

class UMassWarWeaponDefinition;

/**
 * The list of every weapon in the game. A weapon's position in this list (plus one) is the id that is stored per
 * unit and replicated, so: append new weapons at the end, and don't reorder or delete entries in a shipped game.
 * At most 255 weapons.
 */
UCLASS(BlueprintType)
class MASSWARWEAPONS_API UMassWarWeaponCatalog : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapons")
	TArray<TObjectPtr<UMassWarWeaponDefinition>> Weapons;
};

/** Project Settings > Plugins > MassWar Weapons: which catalog the game uses. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "MassWar Weapons"))
class MASSWARWEAPONS_API UMassWarWeaponSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMassWarWeaponSettings() { CategoryName = TEXT("Plugins"); }

	UPROPERTY(EditAnywhere, Config, Category = "Weapons")
	TSoftObjectPtr<UMassWarWeaponCatalog> Catalog;
};
