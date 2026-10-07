// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityElementTypes.h"
#include "MassWarFarAnimationTypes.generated.h"

/** What a baked clip is used for. Several clips may share a role (e.g. three death variants); one is picked per unit. */
UENUM(BlueprintType)
enum class EMassWarFarAnimRole : uint8
{
	/** Standing still. Loops. */
	Idle,
	/** Moving slowly. Loops. */
	Walk,
	/** Moving fast. Loops. */
	Run,
	/** Played once on every landed attack, then back to idle/walk/run. Optional. */
	Attack,
	/** Played once when the unit dies; the last frame is held. Optional (without one a dying far unit is simply hidden). */
	Death
};

/** One clip inside a baked animation texture set (AnimToTexture's start/end frames; both inclusive). */
USTRUCT(BlueprintType)
struct MASSWAREMBODIMENT_API FMassWarFarAnimClip
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|FarAnimation")
	EMassWarFarAnimRole Role = EMassWarFarAnimRole::Idle;

	/** First frame of the clip in the baked texture (the bake asset's Animations[n].StartFrame). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|FarAnimation", meta = (ClampMin = "0"))
	int32 StartFrame = 0;

	/** Last frame of the clip, inclusive (the bake asset's Animations[n].EndFrame). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|FarAnimation", meta = (ClampMin = "0"))
	int32 EndFrame = 0;

	int32 GetNumFrames() const { return FMath::Max(1, EndFrame - StartFrame + 1); }
};

/** The clip table and tuning for one unit type; one shared copy for every unit built from the same config. */
USTRUCT()
struct MASSWAREMBODIMENT_API FMassWarFarAnimationParams : public FMassConstSharedFragment
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FMassWarFarAnimClip> Clips;

	/** Frames per second the animation was baked at (the bake asset's Sample Rate). */
	UPROPERTY()
	float SampleRate = 30.f;

	/** Speed (uu/s) above which a unit counts as moving, and the (lower) speed below which it stops again. */
	UPROPERTY()
	float StartMovingSpeed = 40.f;

	UPROPERTY()
	float StopMovingSpeed = 15.f;

	/** Speed (uu/s) from which the Run clip plays instead of Walk. */
	UPROPERTY()
	float RunSpeed = 300.f;

	/** Number of clips with this role. */
	int32 CountClips(EMassWarFarAnimRole Role) const
	{
		int32 Count = 0;
		for (const FMassWarFarAnimClip& Clip : Clips)
		{
			Count += Clip.Role == Role;
		}
		return Count;
	}

	/** The Nth clip of a role (N wraps around). Null if there is none. */
	const FMassWarFarAnimClip* FindClip(EMassWarFarAnimRole Role, uint32 Pick = 0) const
	{
		const int32 Count = CountClips(Role);
		if (Count == 0)
		{
			return nullptr;
		}
		int32 Remaining = static_cast<int32>(Pick % static_cast<uint32>(Count));
		for (const FMassWarFarAnimClip& Clip : Clips)
		{
			if (Clip.Role == Role && Remaining-- == 0)
			{
				return &Clip;
			}
		}
		return nullptr;
	}
};

/** Per-unit animation state of a far (instanced mesh) unit. Driven by UMassWarUpdateISMProcessor. */
USTRUCT()
struct MASSWAREMBODIMENT_API FMassWarFarAnimationFragment : public FMassFragment
{
	GENERATED_BODY()

	/** The clip being played (copied so the per-frame write does not look anything up). */
	int32 StartFrame = 0;
	int32 NumFrames = 1;
	bool bLoop = true;

	/** Current high-level state (EMassWarFarAnimRole as an int; Idle/Walk/Run/Attack/Death). */
	uint8 State = 0;
	bool bInitialized = false;
	uint8 LastAttackCounter = 0;

	/** World time the current clip started. */
	float StartTime = 0.f;

	/** World time of the last update; a long gap means the unit was shown by an Actor meanwhile. */
	float LastUpdateTime = 0.f;

	/** Speed measured from position changes (smoothed). Velocity is not replicated, so clients cannot use it. */
	float SmoothedSpeed = 0.f;
	bool bMoving = false;
};

/**
 * What the engine's AnimToTexture animation layer (ML_BoneAnimation, AutoPlay on) reads with PerInstanceCustomData, in
 * this order. The layer plays   Frame = StartFrame + Fmod((Time + TimeOffset) * Playrate * SampleRate, EndFrame - StartFrame + 1)
 * and always loops, so a one-shot clip is held on its last frame by sending a one-frame range (StartFrame == EndFrame).
 */
struct FMassWarFarAnimCustomData
{
	float TimeOffset = 0.f;
	float Playrate = 1.f;
	float StartFrame = 0.f;
	float EndFrame = 0.f;
};
