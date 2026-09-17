// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityElementTypes.h"
#include "MassWarClientInterpolationFragment.generated.h"

/**
 * Client-only: the engine's own position/yaw replication (TMassClientBubbleTransformHandler) hard-snaps
 * FTransformFragment straight to whatever the server last sent, which only arrives every
 * FMassReplicationParameters::UpdateInterval (0.1-0.3s) - visually that reads as units teleporting in
 * small jumps rather than moving smoothly. Instead of snapping FTransformFragment directly,
 * FMassWarClientBubbleHandler writes each update's target in here, and
 * UMassWarClientInterpolationProcessor chases it every tick so movement stays smooth between updates.
 * Added by UMassWarReplicationSmoothingTrait; harmless-but-unused if that trait isn't on a config.
 */
USTRUCT()
struct MASSWARREPLICATION_API FMassWarClientInterpolationFragment : public FMassFragment
{
	GENERATED_BODY()

	FVector TargetPosition = FVector::ZeroVector;
	FQuat TargetRotation = FQuat::Identity;

	/** Higher = snaps toward the target faster (less smoothing lag). */
	float InterpSpeed = 8.f;
};
