// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "MassEntityHandle.h"
#include "Orders/MassWarOrderTypes.h"
#include "MassWarOrderSubsystem.generated.h"

class UMassWarOrderDefinition;
class UMassWarOrderBehavior;

/**
 * The project's user-defined orders (Project Settings > MassWar Orders) and the way to give them to units:
 *
 *   FMassWarOrderRequest Request;  Request.Location = A;  Request.Location2 = B;
 *   World->GetSubsystem<UMassWarOrderSubsystem>()->IssueOrder(EntityManager, Unit, PatrolDefinition, Request);
 *
 * Players issue them through the selection plugin's order component, which ends up here. Server / standalone only. A unit carries at
 * most one user-defined order; giving it a new one, or a direct Move / Attack from a player, ends the old one.
 */
UCLASS()
class MASSWAR_API UMassWarOrderSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** The definition / behaviour behind an order id (null for 0 / unknown). */
	const UMassWarOrderDefinition* GetDefinition(uint8 OrderId) const { return Definitions.IsValidIndex(OrderId) ? Definitions[OrderId].Get() : nullptr; }
	const UMassWarOrderBehavior* GetBehavior(uint8 OrderId) const { return Behaviors.IsValidIndex(OrderId) ? Behaviors[OrderId].Get() : nullptr; }

	/** The id of an order definition, 0 if it is null or not listed in the project settings. */
	uint8 FindOrderId(const UMassWarOrderDefinition* Definition) const;

	/** Replaces the order list from code instead of the project setting (tests, mods). Id = position + 1. */
	void SetOrdersOverride(const TArray<UMassWarOrderDefinition*>& NewDefinitions);

	/** Gives a unit an order. Returns false if the unit is dead / has no order fragment or the definition is not listed / has no behaviour. */
	bool IssueOrder(FMassEntityManager& EntityManager, FMassEntityHandle Entity, const UMassWarOrderDefinition* Definition, const FMassWarOrderRequest& Request, bool bPlayerCommanded = true);

	/** Ends a unit's user-defined order (its behaviour gets OnEnded). bStopUnit: also stop the Move / Attack it was carrying out. */
	void CancelOrder(FMassEntityManager& EntityManager, FMassEntityHandle Entity, bool bStopUnit = false);

	/** The context handed to a behaviour's callbacks for this unit (invalid if it carries no order). */
	FMassWarOrderContext MakeContext(FMassEntityManager& EntityManager, FMassEntityHandle Entity, FMassWarCustomOrderFragment& Order) const;

private:
	void Rebuild(const TArray<const UMassWarOrderDefinition*>& NewDefinitions);

	/** Id -> definition / behaviour (index 0 unused). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<const UMassWarOrderDefinition>> Definitions;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMassWarOrderBehavior>> Behaviors;
};