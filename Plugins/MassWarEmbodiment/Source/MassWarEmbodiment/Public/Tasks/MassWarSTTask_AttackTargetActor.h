// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "StateTreeTaskBase.h"
#include "MassWarUnitHandle.h"
#include "MassWarSTTask_AttackTargetActor.generated.h"

class UMassWarUnitStateComponent;

USTRUCT()
struct FMassWarSTTask_AttackTargetActorInstanceData
{
	GENERATED_BODY()

	/** Bind to FMassWarSTEval_FindNearestEnemyActor's NearestEnemy output. */
	UPROPERTY(EditAnywhere, Category = Input)
	FMassWarUnitHandle Target;
};

/**
 * Actor-schema counterpart to MassWarStateTreeAI's FMassWarSTTask_AttackTarget - writes an Attack order
 * via the same FMassWarUnitStateView::RequestAttack. Per the documented Pass 7 scope cut, RequestAttack
 * only actually writes an order when Target is a Mass entity (ordinary Mass units can't be targeted by
 * an embodied unit back yet), so this task can enter Running against a Mass enemy but is a no-op against
 * another embodied Actor.
 */
USTRUCT(meta = (DisplayName = "MassWar Attack Target (Actor)"))
struct MASSWAREMBODIMENT_API FMassWarSTTask_AttackTargetActor : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FMassWarSTTask_AttackTargetActorInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;

	TStateTreeExternalDataHandle<UMassWarUnitStateComponent> StateHandle;
};
