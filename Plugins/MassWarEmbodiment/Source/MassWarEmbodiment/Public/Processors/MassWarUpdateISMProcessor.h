// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassRepresentationTypes.h"
#include "MassWarUpdateISMProcessor.generated.h"

/**
 * Replaces the engine's UMassUpdateISMProcessor (which this plugin switches off in memory at startup): X - pushes each visible far unit's transform to its instanced mesh - and, for units with the Far Animation
 * trait, decides which baked clip the unit plays and pushes that to the instance as per-instance custom data
 * (FMassWarFarAnimCustomData) in the layout the engine's AnimToTexture layer expects. Mass requires custom data to be pushed for every instance on every update, in the
 * same loop as the transform, which is why the stock processor cannot simply be left alone and supplemented.
 *
 * Clip choice, from the unit's own data so every representation agrees:
 *  - Dying -> a Death clip (once, last frame held)
 *  - a landed attack (Core's attack counter changed) -> an Attack clip once, then back to locomotion
 *  - otherwise Idle, Walk or Run by the speed measured from position changes (velocity is not replicated to
 *    clients; hysteresis stops it flickering). Starting clips are phase-shifted per unit so crowds do not move in lockstep.
 *
 * Weapons: a unit with a Weapons loadout and a hand track (Far Animation trait) also gets its active weapon pushed as a second
 * instance, in a mesh set of the weapon alone, at the position of the unit's hand in the frame it is playing (hand track x the
 * body mesh's own placement x the weapon's Attach Offset) - but only within the weapon's LOD significance range: units further
 * out carry nothing. Dying units show no weapon.
 */
UCLASS()
class MASSWAREMBODIMENT_API UMassWarUpdateISMProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassWarUpdateISMProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;

private:
	/** What a weapon looks like when its unit is an instanced mesh; filled in per weapon id, once. */
	struct FWeaponVisual
	{
		enum class EState : uint8 { Unresolved, Pending, Ready, None };
		EState State = EState::Unresolved;
		FStaticMeshInstanceVisualizationDescHandle Handle;
		/** The weapon definition's Attach Offset, and the range of Mass LOD significance it is drawn in. */
		FTransform AttachOffset;
		float MinLODSignificance = 0.f;
		float MaxLODSignificance = 0.f;
	};

	/** Pushes the instance of a unit's weapon (a mesh set of the weapon alone), placed in the unit's hand for the frame it is on. */
	void UpdateInstancedWeapon(FMassEntityHandle Entity, uint8 ActiveWeaponId, const struct FMassWarFarAnimationParams& Params, const struct FMassWarFarAnimCustomData& Anim, float Now,
		const FTransform& Transform, const FTransform& PrevTransform, float LODSignificance, float PrevLODSignificance, const FStaticMeshInstanceVisualizationDesc& BodyDesc,
		FMassInstancedStaticMeshInfoArrayView& Infos, const class UMassWarWeaponSubsystem* Weapons);

	/** Builds the weapon mesh sets requested during this frame's loop; runs after it, when the engine's mesh list is not being read. */
	void CreatePendingWeaponDescs(const class UMassWarWeaponSubsystem* Weapons);

	/** Indexed by weapon id (0 = no weapon, unused). */
	TArray<FWeaponVisual> WeaponVisuals;
	TArray<uint8> PendingWeapons;
	TWeakObjectPtr<class UMassRepresentationSubsystem> PendingSubsystem;
};
