// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "MassEntityHandle.h"
#include "Fragments/MassWarUnitFragments.h"
#include "MassWarWeaponSubsystem.generated.h"

class UMassWarWeaponDefinition;
class UMassWarWeaponCatalog;
struct FMassEntityManager;

/** The weapons a unit should carry, by definition - what you hand over when you spawn a unit. Any slot may be empty. */
USTRUCT(BlueprintType)
struct MASSWARWEAPONS_API FMassWarLoadout
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	TObjectPtr<UMassWarWeaponDefinition> Primary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	TObjectPtr<UMassWarWeaponDefinition> Secondary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
	TObjectPtr<UMassWarWeaponDefinition> Special;

	UMassWarWeaponDefinition* Get(EMassWarWeaponSlot Slot) const;
};

/**
 * Turns the project's weapon catalog (Project Settings > Plugins > MassWar Weapons) into ids and back, and is how
 * code hands a unit its weapons:
 *  - at spawn: ApplyLoadout(EntityManager, Entity, Loadout) right after spawning (like setting team and owner), or let
 *    the unit type's "MassWar Weapons" trait supply a default loadout;
 *  - later: SetActiveSlot to draw a different weapon.
 * Exists in every world and net mode (clients need it to resolve replicated ids to meshes).
 */
UCLASS()
class MASSWARWEAPONS_API UMassWarWeaponSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** The weapon behind an id from a loadout fragment (null for 0 / unknown). */
	const UMassWarWeaponDefinition* GetDefinition(uint8 WeaponId) const;

	/** The id of a weapon, 0 if it is null or not in the catalog. */
	uint8 FindId(const UMassWarWeaponDefinition* Definition) const;

	/** The weapon currently in hand (null if none). */
	const UMassWarWeaponDefinition* GetActiveWeapon(const FMassWarLoadoutFragment& Loadout) const { return GetDefinition(Loadout.GetActiveWeaponId()); }

	/** A loadout fragment for the given weapons; the first non-empty slot (primary, secondary, special) is in hand. */
	FMassWarLoadoutFragment MakeFragment(const FMassWarLoadout& Loadout) const;

	/** Gives an existing unit these weapons (it must have the MassWar Weapons trait). Returns false if it has no loadout. */
	bool ApplyLoadout(FMassEntityManager& EntityManager, FMassEntityHandle Entity, const FMassWarLoadout& Loadout) const;

	/** Takes the weapon in this slot into hand. Returns false if the unit has no loadout or the slot is empty. */
	bool SetActiveSlot(FMassEntityManager& EntityManager, FMassEntityHandle Entity, EMassWarWeaponSlot Slot) const;

	/** Replaces the catalog from code instead of the project setting (tests, mods). */
	void SetCatalogOverride(UMassWarWeaponCatalog* Catalog);

private:
	void RebuildLookup();

	UPROPERTY(Transient)
	TObjectPtr<UMassWarWeaponCatalog> Catalog;

	/** Id (index + 1) -> definition, built from the catalog. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<const UMassWarWeaponDefinition>> ById;
};
