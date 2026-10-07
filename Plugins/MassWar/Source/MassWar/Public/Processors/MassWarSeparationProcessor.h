// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MassWarSeparationProcessor.generated.h"

/**
 * Unit-vs-unit avoidance: overlapping units push each other apart so crowds spread out instead of stacking
 * on one spot. Deliberately simple and cheap for very large crowds - no prediction, no per-pair state:
 *  - every frame all living avoiding units are put into a hashed uniform grid (O(n), no sorting),
 *  - each unit sums a push away from every neighbour whose personal-space circle it overlaps (parallel),
 *  - the push is added on top of the steering velocity MassWarOrderMovementProcessor just wrote.
 * Moving units also sidestep a little (always to their own right) so two units meeting head-on pass each
 * other instead of deadlocking. Standing units are only nudged when properly overlapped, so a firing line
 * at AttackStopDistance doesn't jitter. A Move order whose unit is blocked by standing units near the
 * destination completes early - otherwise a crowd sent to one point would push against each other forever.
 * Dying units take no part. Server/Standalone only, like all MassWar movement; clients receive positions.
 */
UCLASS()
class MASSWAR_API UMassWarSeparationProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassWarSeparationProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;

private:
	// Scratch buffers kept between frames so steady-state execution allocates nothing.
	TArray<FVector2f> Positions;
	TArray<float> Radii;
	TArray<uint8> Flags; // bit 0: takes part, bit 1: moving
	TArray<int32> CellX;
	TArray<int32> CellY;
	TArray<int32> Next;
	TArray<int32> Head;
	TArray<FVector2f> Push;
	TArray<uint8> Blocked;
};
