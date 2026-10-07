// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "MassWarLineOfSightSubsystem.generated.h"

/**
 * Shared "can A see B" service for everything that needs real line of sight (MassWarPerception's sight,
 * MassWarFogOfWar's team visibility). A ray from eye height to a point on the target, on the Visibility
 * channel: level geometry blocks it, unit puppets (no collision) do not.
 *
 * Raycasts are game-thread-only and far from free, so callers ask for a per-frame budget first
 * (MassWar.LOS.RaycastsPerFrame): when the budget is spent they keep using what they last knew instead of
 * casting, which spreads a big crowd's checks over several frames instead of stalling one.
 */
UCLASS()
class MASSWAR_API UMassWarLineOfSightSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** True while this frame's raycast budget is not yet used up. */
	bool HasBudget();

	/** Game thread only. Casts one ray (using up one unit of this frame's budget) and returns true if the target is visible. */
	bool TraceLineOfSight(const FVector& ViewerLocation, const FVector& TargetLocation);

private:
	uint64 BudgetFrame = 0;
	int32 UsedThisFrame = 0;
};
