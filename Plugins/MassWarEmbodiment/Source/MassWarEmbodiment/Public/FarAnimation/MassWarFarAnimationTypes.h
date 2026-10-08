// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityElementTypes.h"
#include "Weapons/MassWarWeaponDefinition.h"
#include "MassWarFarAnimationTypes.generated.h"

class UAnimSequence;

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

	/** The weapon stance this clip is for. A unit uses the clips matching the hold type of the weapon in its hand
	 *  (Rifle clips for a rifleman); where there are none it falls back to the "Unarmed" ones, so a table that only
	 *  has one set of clips (all left on Unarmed) keeps working for every unit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|FarAnimation")
	EMassWarWeaponHoldType HoldType = EMassWarWeaponHoldType::Unarmed;

	/** First frame of the clip in the baked texture (the bake asset's Animations[n].StartFrame). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|FarAnimation", meta = (ClampMin = "0"))
	int32 StartFrame = 0;

	/** Last frame of the clip, inclusive (the bake asset's Animations[n].EndFrame). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|FarAnimation", meta = (ClampMin = "0"))
	int32 EndFrame = 0;

	/** The animation this clip was baked from (the bake asset's Animations[n]). Optional: only used to find where the unit's hand is,
	 *  so weapons can follow it (see the trait's Hand Bone). Clips without one carry no weapon. Baked from its start, at the trait's Sample Rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|FarAnimation")
	TObjectPtr<UAnimSequence> Animation;

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

	/** The hand bone's transform in mesh space (the space the baked body is in) for every baked frame, indexed by the bake's
	 *  frame number. A frame with no source animation holds a zero-scale transform. Empty = weapons do not follow a hand. */
	UPROPERTY()
	TArray<FTransform> HandTrack;

	/** The hand's transform at a (fractional) frame of the clip [ClipStart, ClipEnd], blending to the next frame and wrapping
	 *  at the clip's end the way the playing layer does. False if the track has no data there. */
	bool SampleHand(float Frame, int32 ClipStart, int32 ClipEnd, FTransform& OutHand) const
	{
		const int32 A = FMath::FloorToInt(Frame);
		if (!HandTrack.IsValidIndex(A) || HandTrack[A].GetScale3D().IsNearlyZero())
		{
			return false;
		}
		const float Alpha = Frame - static_cast<float>(A);
		const int32 B = A >= ClipEnd ? ClipStart : A + 1;
		if (Alpha < 0.01f || !HandTrack.IsValidIndex(B) || HandTrack[B].GetScale3D().IsNearlyZero())
		{
			OutHand = HandTrack[A];
			return true;
		}
		const FTransform& From = HandTrack[A];
		const FTransform& To = HandTrack[B];
		OutHand = FTransform(FQuat::FastLerp(From.GetRotation(), To.GetRotation(), Alpha).GetNormalized(),
			FMath::Lerp(From.GetTranslation(), To.GetTranslation(), Alpha));
		return true;
	}

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

	/** A clip for a role in the stance of the given hold type: one tagged with that hold type, else a generic
	 *  (Unarmed-tagged) one, else any clip of the role. The Nth of the candidates (N wraps around). Null if the role has none. */
	const FMassWarFarAnimClip* FindClipFor(EMassWarFarAnimRole Role, EMassWarWeaponHoldType HoldType, uint32 Pick = 0) const
	{
		for (const EMassWarWeaponHoldType Wanted : { HoldType, EMassWarWeaponHoldType::Unarmed })
		{
			int32 Count = 0;
			for (const FMassWarFarAnimClip& Clip : Clips)
			{
				Count += (Clip.Role == Role && Clip.HoldType == Wanted);
			}
			if (Count > 0)
			{
				int32 Remaining = static_cast<int32>(Pick % static_cast<uint32>(Count));
				for (const FMassWarFarAnimClip& Clip : Clips)
				{
					if (Clip.Role == Role && Clip.HoldType == Wanted && Remaining-- == 0)
					{
						return &Clip;
					}
				}
			}
		}
		return FindClip(Role, Pick);
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

	/** Weapon state. LastActiveWeaponId: the weapon id last looked at (0xFF = none yet), so the catalog is only consulted when
	 *  it changes; HoldType: the stance of that weapon (EMassWarWeaponHoldType); ClipHoldType: the stance the playing clip was
	 *  picked for. */
	uint8 LastActiveWeaponId = 0xFF;
	uint8 HoldType = 0;
	uint8 ClipHoldType = 0;
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
