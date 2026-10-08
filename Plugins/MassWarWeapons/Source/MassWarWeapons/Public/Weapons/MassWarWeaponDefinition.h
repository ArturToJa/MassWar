// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "Fragments/MassWarUnitFragments.h"
#include "MassWarWeaponDefinition.generated.h"

class UStaticMesh;
class UAnimMontage;
class UNiagaraSystem;

/**
 * How a unit holds a weapon - the one thing an animation blueprint needs to pick the right locomotion, idle and
 * aim animations (rifle stance, pistol stance, ...). Exposed to the anim blueprint as ActiveWeaponHoldType.
 */
UENUM(BlueprintType)
enum class EMassWarWeaponHoldType : uint8
{
	/** Nothing in hand. */
	Unarmed,
	Pistol,
	Rifle,
	/** Shoulder-fired heavy weapon such as a rocket launcher. */
	Launcher
};

/**
 * One kind of weapon, authored as a data asset in the game project (the plugin ships none) and listed in the
 * project's weapon catalog. Units refer to weapons by definition when they are spawned; at runtime only the
 * catalog index travels (a byte per slot), so definitions must be identical on server and clients - they are,
 * being the same assets.
 */
UCLASS(BlueprintType)
class MASSWARWEAPONS_API UMassWarWeaponDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Shown in tools and logs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FText DisplayName;

	/** Which slot this weapon is carried in. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EMassWarWeaponSlot Slot = EMassWarWeaponSlot::Primary;

	/** The stance animations to use while this weapon is in hand. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	EMassWarWeaponHoldType HoldType = EMassWarWeaponHoldType::Rifle;

	// ---- Visuals ----

	/** The weapon's mesh, shown in the unit's hand while it is the active weapon. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UStaticMesh> Mesh;

	/** Where the mesh sits relative to the unit's weapon socket (set on the unit's visual actor) - adjust until the
	 *  grip lines up with the hand. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FTransform AttachOffset;

	/** Attack animations for this weapon, played one per shot (picked round-robin); if empty, the unit visual's own
	 *  attack montages are used. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TArray<TObjectPtr<UAnimMontage>> AttackMontages;

	// ---- Muzzle effect (played on every shot) ----

	/** Niagara effect played at the muzzle each time the unit fires - leave empty for none. Played only while the
	 *  unit is shown as a full actor (close to the camera). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Muzzle Effect")
	TSoftObjectPtr<UNiagaraSystem> MuzzleEffect;

	/** Socket on the weapon mesh to play the effect at. If the mesh has no such socket (or this is empty),
	 *  MuzzleOffset is used relative to the weapon mesh origin. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Muzzle Effect")
	FName MuzzleSocketName = TEXT("Muzzle");

	/** Effect placement relative to the socket (or to the mesh origin when there is no socket). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Muzzle Effect")
	FTransform MuzzleOffset;

	/** Loaded on first use, then kept (a weapon is only ever needed on machines that show one). */
	UStaticMesh* LoadMesh() const;
	UNiagaraSystem* LoadMuzzleEffect() const;
};
