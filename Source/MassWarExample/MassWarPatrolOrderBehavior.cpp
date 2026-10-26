// Copyright Epic Games, Inc. All Rights Reserved.

#include "MassWarPatrolOrderBehavior.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassWarPatrolOrderBehavior)

FVector UMassWarPatrolOrderBehavior::GetWaypoint(const FMassWarOrderContext& Context)
{
	return UMassWarOrderLibrary::GetStage(Context) == 0 ? UMassWarOrderLibrary::GetOrderLocation(Context) : UMassWarOrderLibrary::GetOrderLocation2(Context);
}

void UMassWarPatrolOrderBehavior::OnStarted_Implementation(const FMassWarOrderContext& Context) const
{
	// No second point given: patrol back to where the unit is now.
	if (Context.Order->Location2.IsNearlyZero())
	{
		Context.Order->Location2 = UMassWarOrderLibrary::GetUnitLocation(Context);
	}
	UMassWarOrderLibrary::SetStage(Context, 0);
	UMassWarOrderLibrary::MoveTo(Context, GetWaypoint(Context));
}

void UMassWarPatrolOrderBehavior::OnTick_Implementation(const FMassWarOrderContext& Context) const
{
	// Safety net: if the unit stands idle with nothing handed over (something else cleared its move), send it on its way again.
	if (!Context.Order->bAwaitingPrimitive)
	{
		UMassWarOrderLibrary::MoveTo(Context, GetWaypoint(Context));
	}
}

void UMassWarPatrolOrderBehavior::OnPrimitiveFinished_Implementation(const FMassWarOrderContext& Context) const
{
	// Reached the point: turn around. Otherwise the leg was interrupted (a fight, a blocked path): carry on with the same leg.
	if (UMassWarOrderLibrary::GetDistanceToLocation(Context, GetWaypoint(Context)) <= ArrivalRadius)
	{
		UMassWarOrderLibrary::SetStage(Context, UMassWarOrderLibrary::GetStage(Context) == 0 ? 1 : 0);
	}
	UMassWarOrderLibrary::MoveTo(Context, GetWaypoint(Context));
}
