// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityElementTypes.h"
#include "MassEntityHandle.h"
#include "Fragments/MassWarUnitFragments.h"
#include "MassWarOrderTypes.generated.h"

class UMassWarOrderDefinition;
struct FMassEntityManager;

/**
 * What a player or script asks a unit to do with a user-defined order: where, against whom, and one free number. What the numbers
 * mean is up to the order's behaviour class (a patrol uses Location and Location2 as its two end points; a "retreat" might use
 * Param as the distance).
 */
USTRUCT(BlueprintType)
struct MASSWAR_API FMassWarOrderRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Orders")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Orders")
	FVector Location2 = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Orders")
	float Param = 0.f;

	/** A unit this order is about (an escort target, a unit to follow...). Optional. */
	UPROPERTY()
	FMassEntityHandle Target;
};

/** The user-defined order a unit is carrying out, if any. The built-in Idle / Move / Attack stay in FMassWarOrderFragment; a user order drives those. */
USTRUCT()
struct MASSWAR_API FMassWarCustomOrderFragment : public FMassFragment
{
	GENERATED_BODY()

	/** The order's id (its position in the order list in the project settings, plus one); 0 = no user-defined order. */
	UPROPERTY(Transient)
	uint8 OrderId = 0;

	/** Free state byte for the behaviour (which waypoint the patrol is heading for...). Reset to 0 when the order starts. */
	UPROPERTY(Transient)
	uint8 Stage = 0;

	/** The order has handed the unit a Move / Attack and is waiting for it to end. */
	UPROPERTY(Transient)
	bool bAwaitingPrimitive = false;

	/** From the order's definition: keep the AI's automatic engagement from overriding this order's Move / Attack. */
	UPROPERTY(Transient)
	bool bBlocksAutoEngage = false;

	/** World time at which the behaviour's tick runs next. */
	UPROPERTY(Transient)
	float NextTickTime = 0.f;

	UPROPERTY(Transient)
	float Param = 0.f;

	UPROPERTY(Transient)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector Location2 = FVector::ZeroVector;

	UPROPERTY(Transient)
	FMassEntityHandle Target;

	bool IsActive() const { return OrderId != 0; }

	void Reset() { *this = FMassWarCustomOrderFragment(); }
};

/**
 * Handed to an order behaviour's callbacks: which unit and which order, and (through the UMassWarOrderLibrary functions in Blueprint, or
 * directly in C++) everything needed to look at the unit and drive it. Only valid during the callback it is passed to - do not store it.
 */
USTRUCT(BlueprintType)
struct MASSWAR_API FMassWarOrderContext
{
	GENERATED_BODY()

	FMassEntityManager* EntityManager = nullptr;
	FMassEntityHandle Entity;
	const UMassWarOrderDefinition* Definition = nullptr;
	FMassWarCustomOrderFragment* Order = nullptr;
	UWorld* World = nullptr;

	bool IsValid() const { return EntityManager && Entity.IsValid() && Order; }
};


/**
 * Called where a built-in Move / Attack ends and the "player commanded" mark would be cleared (so the AI's automatic engagement may take over
 * again). Skipped while a user-defined order that blocks auto-engage is still in charge: it hands the unit its next Move itself, and the unit
 * must not be fair game for the AI in between.
 */
inline void MassWarClearPlayerCommanded(FMassWarOrderFragment& Order, const FMassWarCustomOrderFragment* Custom)
{
	if (!Custom || !Custom->IsActive() || !Custom->bBlocksAutoEngage)
	{
		Order.bPlayerCommanded = false;
	}
}