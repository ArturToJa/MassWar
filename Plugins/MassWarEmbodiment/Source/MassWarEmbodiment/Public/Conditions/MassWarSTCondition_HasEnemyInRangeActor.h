// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "StateTreeConditionBase.h"
#include "MassWarSTCondition_HasEnemyInRangeActor.generated.h"

class UMassWarUnitStateComponent;

USTRUCT()
struct FMassWarSTCondition_HasEnemyInRangeActorInstanceData
{
	GENERATED_BODY()

	/** Bind to FMassWarSTEval_FindNearestEnemyActor's outputs. */
	UPROPERTY(EditAnywhere, Category = Input)
	bool bHasEnemy = false;

	UPROPERTY(EditAnywhere, Category = Input)
	float DistanceToEnemy = 0.f;

	/** Engagement distance. Keep in sync with the Mass-schema condition's Range when authoring a unit's
	 *  StateTree/traits. */
	UPROPERTY(EditAnywhere, Category = Parameter)
	float Range = 1000.f;
};

/**
 * Actor-schema counterpart to MassWarStateTreeAI's FMassWarSTCondition_HasEnemyInRange - true if the
 * bound nearest-enemy is present and within Range, and the unit isn't currently fulfilling a
 * player-issued order (UMassWarUnitStateComponent::Order.bPlayerCommanded).
 */
USTRUCT(meta = (DisplayName = "MassWar Has Enemy In Range (Actor)"))
struct MASSWAREMBODIMENT_API FMassWarSTCondition_HasEnemyInRangeActor : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FMassWarSTCondition_HasEnemyInRangeActorInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

	TStateTreeExternalDataHandle<UMassWarUnitStateComponent> StateHandle;
};
