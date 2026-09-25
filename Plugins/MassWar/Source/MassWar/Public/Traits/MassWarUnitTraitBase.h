// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityTraitBase.h"
#include "MassWarUnitTraitBase.generated.h"

/**
 * Base trait for every MassWar unit. Adds the Core-owned identity fragments (Team, Order, stable
 * network id) that every other MassWar plugin builds on. Combine with an engine movement trait and an
 * engine representation trait on the same UMassEntityConfigAsset to get a spawnable unit type; add
 * MassWarCombat/MassWarStateTreeAI/MassWarReplication traits later to layer in more behavior.
 */
UCLASS(meta = (DisplayName = "MassWar Unit"))
class MASSWAR_API UMassWarUnitTraitBase : public UMassEntityTraitBase
{
	GENERATED_BODY()

public:
	/** Starting team id for units spawned from this config. 0 = neutral/unassigned. */
	UPROPERTY(EditAnywhere, Category = "MassWar")
	uint8 DefaultTeamId = 1;

	UPROPERTY(EditAnywhere, Category = "MassWar|Movement")
	float MoveSpeed = 500.f;

	/** Move orders: how close to the destination counts as arrived. Keep small - see
	 *  FMassWarMovementParamsFragment::AcceptanceRadius. */
	UPROPERTY(EditAnywhere, Category = "MassWar|Movement")
	float AcceptanceRadius = 50.f;

	/** Attack orders: distance from the target at which the unit stops closing in - see
	 *  FMassWarMovementParamsFragment::AttackStopDistance. Keep within the unit's attack range. */
	UPROPERTY(EditAnywhere, Category = "MassWar|Movement")
	float AttackStopDistance = 100.f;

	/** Route around static obstacles using the level's navigation mesh instead of moving in a straight line.
	 *  Needs a NavMeshBoundsVolume (and a built navmesh) in the level; without a navmesh units fall back to
	 *  straight-line movement. Covers static geometry only - see bAvoidOtherUnits for unit-vs-unit. */
	UPROPERTY(EditAnywhere, Category = "MassWar|Navigation")
	bool bUseNavMesh = false;

	/** Navmesh routing: minimum seconds between path requests for a unit chasing a moving target. Higher is
	 *  cheaper, lower reacts faster. */
	UPROPERTY(EditAnywhere, Category = "MassWar|Navigation", meta = (EditCondition = "bUseNavMesh", ClampMin = "0.1"))
	float NavRepathInterval = 1.f;

	/** Navmesh routing: how far (units) a chased target must move before the path is recomputed. */
	UPROPERTY(EditAnywhere, Category = "MassWar|Navigation", meta = (EditCondition = "bUseNavMesh", ClampMin = "50.0"))
	float NavRepathDistance = 300.f;

	/** Push apart from other units instead of stacking on top of them (cheap local separation, see
	 *  UMassWarSeparationProcessor). Units without this pass through each other. */
	UPROPERTY(EditAnywhere, Category = "MassWar|Avoidance")
	bool bAvoidOtherUnits = true;

	/** Personal-space radius in uu; roughly half the width of the unit's body. Keep it below half of
	 *  AttackStopDistance or melee units cannot get close enough to fight. */
	UPROPERTY(EditAnywhere, Category = "MassWar|Avoidance", meta = (EditCondition = "bAvoidOtherUnits", ClampMin = "1.0"))
	float AvoidanceRadius = 40.f;

	/** Push strength as a fraction of MoveSpeed at full overlap. Higher spreads crowds faster but jostles more. */
	UPROPERTY(EditAnywhere, Category = "MassWar|Avoidance", meta = (EditCondition = "bAvoidOtherUnits", ClampMin = "0.0", ClampMax = "2.0"))
	float AvoidanceStrength = 1.f;

	/** How long (seconds) a dead unit stays around as a "dying" entity before it is destroyed - set it to
	 *  the length of your death animation. See FMassWarLifeFragment. */
	UPROPERTY(EditAnywhere, Category = "MassWar|Life", meta = (ClampMin = "0.0"))
	float DeathLingerTime = 3.f;

	/** Yaw turn speed in degrees/second - see FMassWarMovementParamsFragment::TurnRate. */
	UPROPERTY(EditAnywhere, Category = "MassWar|Movement")
	float TurnRate = 720.f;

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
