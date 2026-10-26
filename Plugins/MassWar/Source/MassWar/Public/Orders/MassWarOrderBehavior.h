// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "Orders/MassWarOrderTypes.h"
#include "MassWarOrderBehavior.generated.h"

/**
 * What a user-defined order does. One instance per order definition, shared by every unit carrying that order, so keep it stateless:
 * a unit's own state lives in its order (Stage, Param, the locations). A behaviour does not move units itself - it hands them the
 * built-in Move / Attack / Idle (MoveTo, AttackTarget, Stop in UMassWarOrderLibrary, or directly in C++), so movement, pathing,
 * avoidance and combat work unchanged underneath.
 *
 * Callbacks run on the game thread, on the server, and only for the units that carry the order:
 *  - OnStarted:           once, when the unit is given the order.
 *  - OnTick:              every TickInterval seconds (spread over units).
 *  - OnPrimitiveFinished: the Move / Attack the order handed over has ended (arrived, target dead, or interrupted by the AI's own
 *                         engagement) and the unit is idle again.
 *  - OnEnded:             the order was cancelled, replaced by another, or the unit died. Not called after Finish().
 * Call Finish when the order is complete.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew)
class MASSWAR_API UMassWarOrderBehavior : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "MassWar|Orders")
	void OnStarted(const FMassWarOrderContext& Context) const;
	virtual void OnStarted_Implementation(const FMassWarOrderContext& Context) const {}

	UFUNCTION(BlueprintNativeEvent, Category = "MassWar|Orders")
	void OnTick(const FMassWarOrderContext& Context) const;
	virtual void OnTick_Implementation(const FMassWarOrderContext& Context) const {}

	UFUNCTION(BlueprintNativeEvent, Category = "MassWar|Orders")
	void OnPrimitiveFinished(const FMassWarOrderContext& Context) const;
	virtual void OnPrimitiveFinished_Implementation(const FMassWarOrderContext& Context) const {}

	UFUNCTION(BlueprintNativeEvent, Category = "MassWar|Orders")
	void OnEnded(const FMassWarOrderContext& Context) const;
	virtual void OnEnded_Implementation(const FMassWarOrderContext& Context) const {}
};

/** Blueprint (and C++) helpers for order behaviours. */
UCLASS()
class MASSWAR_API UMassWarOrderLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	//~ Driving the unit

	/** Hand the unit a Move to Location. bStopAtAttackDistance: count it as arrived once within the attack stop distance (an approach), not on the spot. */
	UFUNCTION(BlueprintCallable, Category = "MassWar|Orders")
	static void MoveTo(const FMassWarOrderContext& Context, FVector Location, bool bStopAtAttackDistance = false);

	/** Hand the unit an Attack on the order's target (the request's Target). False if there is none or it is gone. */
	UFUNCTION(BlueprintCallable, Category = "MassWar|Orders")
	static bool AttackOrderTarget(const FMassWarOrderContext& Context);

	/** Stop whatever Move / Attack the order handed the unit. */
	UFUNCTION(BlueprintCallable, Category = "MassWar|Orders")
	static void Stop(const FMassWarOrderContext& Context);

	/** The order is complete: the unit stops and carries no order any more. OnEnded is not called. */
	UFUNCTION(BlueprintCallable, Category = "MassWar|Orders")
	static void Finish(const FMassWarOrderContext& Context);

	//~ Looking at the unit and the order

	UFUNCTION(BlueprintPure, Category = "MassWar|Orders")
	static FVector GetUnitLocation(const FMassWarOrderContext& Context);

	UFUNCTION(BlueprintPure, Category = "MassWar|Orders")
	static FVector GetOrderLocation(const FMassWarOrderContext& Context);

	UFUNCTION(BlueprintPure, Category = "MassWar|Orders")
	static FVector GetOrderLocation2(const FMassWarOrderContext& Context);

	UFUNCTION(BlueprintPure, Category = "MassWar|Orders")
	static float GetOrderParam(const FMassWarOrderContext& Context);

	/** Horizontal distance from the unit to a location. */
	UFUNCTION(BlueprintPure, Category = "MassWar|Orders")
	static float GetDistanceToLocation(const FMassWarOrderContext& Context, FVector Location);

	/** The order's own state byte (0 when the order starts). */
	UFUNCTION(BlueprintPure, Category = "MassWar|Orders")
	static int32 GetStage(const FMassWarOrderContext& Context);

	UFUNCTION(BlueprintCallable, Category = "MassWar|Orders")
	static void SetStage(const FMassWarOrderContext& Context, int32 Stage);

	/** Whether the order's target unit is alive; and where it is. */
	UFUNCTION(BlueprintPure, Category = "MassWar|Orders")
	static bool IsOrderTargetAlive(const FMassWarOrderContext& Context);

	UFUNCTION(BlueprintPure, Category = "MassWar|Orders")
	static FVector GetOrderTargetLocation(const FMassWarOrderContext& Context);

	/** The unit's value of a stat (see MassWarStatSettings). */
	UFUNCTION(BlueprintPure, Category = "MassWar|Orders")
	static float GetUnitStat(const FMassWarOrderContext& Context, FGameplayTag StatTag);
};
