// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassStateTreeTypes.h"
#include "MassWarSTCondition_HasEnemyInRange.generated.h"

struct FMassWarOrderFragment;

USTRUCT()
struct FMassWarSTCondition_HasEnemyInRangeInstanceData
{
	GENERATED_BODY()

	/** Bind to FMassWarSTEval_FindNearestEnemy's outputs. */
	UPROPERTY(EditAnywhere, Category = Input)
	bool bHasEnemy = false;

	UPROPERTY(EditAnywhere, Category = Input)
	float DistanceToEnemy = 0.f;

	/** Engagement distance. Deliberately independent from MassWarCombat's AttackRange (this plugin
	 *  doesn't depend on MassWarCombat) - keep them in sync when authoring a unit's StateTree/traits. */
	UPROPERTY(EditAnywhere, Category = Parameter)
	float Range = 1000.f;
};

/**
 * True if the bound nearest-enemy is both present and within Range - AND the unit isn't currently
 * fulfilling a player-issued order (FMassWarOrderFragment::bPlayerCommanded). A player's move/attack
 * order takes priority over auto-engage until it naturally completes (arrival, or the attack target
 * dies/becomes invalid), at which point bPlayerCommanded clears itself and normal AI behavior resumes.
 */
USTRUCT(meta = (DisplayName = "MassWar Has Enemy In Range"))
struct MASSWARSTATETREEAI_API FMassWarSTCondition_HasEnemyInRange : public FMassStateTreeConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FMassWarSTCondition_HasEnemyInRangeInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

	TStateTreeExternalDataHandle<FMassWarOrderFragment> OrderHandle;
};
