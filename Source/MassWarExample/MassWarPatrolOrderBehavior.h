// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Orders/MassWarOrderBehavior.h"
#include "MassWarPatrolOrderBehavior.generated.h"

/**
 * The example project's Patrol order: the unit walks back and forth between two points until it is given another order.
 *  - Location is the far point; Location2 the near one. If Location2 is left at zero, the unit patrols back to where it stood when
 *    it got the order (so "Patrol to here" on the map is enough).
 *  - If something interrupts a leg (the AI stops to fight what it meets - turn off "Blocks Auto Engage" on the order definition to
 *    allow that), the unit resumes the same leg afterwards instead of skipping a point.
 *
 * To use it: create an Order Definition data asset with this class as its Behavior, list it in Project Settings > MassWar Orders, and
 * give the order with the player controller's Issue Custom Order To Selection / At Cursor.
 */
UCLASS(meta = (DisplayName = "Patrol"))
class MASSWAREXAMPLE_API UMassWarPatrolOrderBehavior : public UMassWarOrderBehavior
{
	GENERATED_BODY()

public:
	/** A point counts as reached when the unit is within this distance of it. */
	UPROPERTY(EditDefaultsOnly, Category = "Patrol", meta = (ClampMin = "10.0"))
	float ArrivalRadius = 250.f;

	virtual void OnStarted_Implementation(const FMassWarOrderContext& Context) const override;
	virtual void OnTick_Implementation(const FMassWarOrderContext& Context) const override;
	virtual void OnPrimitiveFinished_Implementation(const FMassWarOrderContext& Context) const override;

private:
	/** The point the unit is heading for: Stage 0 = Location, Stage 1 = Location2. */
	static FVector GetWaypoint(const FMassWarOrderContext& Context);
};
