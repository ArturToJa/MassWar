// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "Engine/DeveloperSettings.h"
#include "MassWarOrderDefinition.generated.h"

class UMassWarOrderBehavior;

/**
 * One kind of order the project adds on top of the built-in Move and Attack: Patrol, Hold Position, Retreat, Guard... Authored as a
 * data asset in the game project (the plugin ships none) and listed in Project Settings > MassWar Orders; an order's id is its
 * position in that list plus one, so only ever append to the list.
 */
UCLASS(BlueprintType)
class MASSWAR_API UMassWarOrderDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Shown in tools, logs and UI. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Order")
	FText DisplayName;

	/** What the order does: a subclass of MassWarOrderBehavior (C++ or Blueprint). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Order")
	TSubclassOf<UMassWarOrderBehavior> Behavior;

	/** Seconds between the behaviour's tick for one unit. Ticks are spread over time, never all at once; keep it as high as the order can bear. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Order", meta = (ClampMin = "0.05"))
	float TickInterval = 0.25f;

	/** Whether players may give this order (otherwise only code and AI can). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Order")
	bool bPlayerIssuable = true;

	/** While the order is active, the AI's automatic engagement does not override the moves and attacks it gives. Off: the unit may stop to
	 *  fight what it meets and the behaviour is told when that ends (a patrol that engages enemies on its way). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Order")
	bool bBlocksAutoEngage = false;

	/** Orders given to a formation: every member keeps its place relative to the formation's centre (the order's locations are shifted by it).
	 *  Off: every member gets the same locations. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Order")
	bool bKeepFormationOffsets = true;
};

/** Project Settings > Plugins > MassWar Orders. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "MassWar Orders"))
class MASSWAR_API UMassWarOrderSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMassWarOrderSettings() { CategoryName = TEXT("Plugins"); }

	/** Every user-defined order of the game. Append only: a position (plus one) is the order's id, used by saved data and over the network. At most 255. */
	UPROPERTY(EditAnywhere, Config, Category = "Orders")
	TArray<TSoftObjectPtr<UMassWarOrderDefinition>> Orders;
};
