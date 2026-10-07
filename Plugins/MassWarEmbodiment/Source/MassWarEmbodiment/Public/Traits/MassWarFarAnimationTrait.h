// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityTraitBase.h"
#include "FarAnimation/MassWarFarAnimationTypes.h"
#include "MassWarFarAnimationTrait.generated.h"

/**
 * Animates a unit while it is shown as an instanced static mesh (far LOD) by playing a baked animation texture
 * (engine AnimToTexture, bone mode) in the material. Per instance, every frame, the update step writes the four
 * numbers the engine's own animation layer (ML_BoneAnimation, AutoPlay on) reads as PerInstanceCustomData 0..3 - time
 * offset, play rate, start frame, end frame - see FMassWarFarAnimCustomData; the material needs no changes.
 * Which clip plays comes from the unit's own data (moving speed, attack counter,
 * life state), so near (Actor) and far (instance) representations always agree.
 *
 * Content-free: the clip table below is filled in by the project from its own bake. Needs a visualization trait
 * whose static mesh is the baked mesh with the animation material, and MassWar (Core)'s unit trait.
 */
UCLASS(meta = (DisplayName = "MassWar Far Animation"))
class MASSWAREMBODIMENT_API UMassWarFarAnimationTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

public:
	/** The clips of the bake and what each is for. Frame numbers are the bake asset's start/end frames. Death and
	 *  Attack clips are optional; if there are several of a role one is picked per unit. */
	UPROPERTY(EditAnywhere, Category = "MassWar|FarAnimation", meta = (TitleProperty = "Role"))
	TArray<FMassWarFarAnimClip> Clips;

	/** Frames per second the animation was baked at (the bake asset's Sample Rate). */
	UPROPERTY(EditAnywhere, Category = "MassWar|FarAnimation", meta = (ClampMin = "1.0"))
	float SampleRate = 30.f;

	/** A unit moving faster than this (uu/s) starts walking/running; it stops once it is slower than StopMovingSpeed. */
	UPROPERTY(EditAnywhere, Category = "MassWar|FarAnimation", meta = (ClampMin = "0.0"))
	float StartMovingSpeed = 40.f;

	UPROPERTY(EditAnywhere, Category = "MassWar|FarAnimation", meta = (ClampMin = "0.0"))
	float StopMovingSpeed = 15.f;

	/** From this speed (uu/s) the Run clip plays instead of Walk. */
	UPROPERTY(EditAnywhere, Category = "MassWar|FarAnimation", meta = (ClampMin = "0.0"))
	float RunSpeed = 300.f;

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
