// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassStateTreeTypes.h"
#include "MassWarUnitHandle.h"
#include "MassWarSTTask_AttackTarget.generated.h"

struct FMassWarOrderFragment;

USTRUCT()
struct FMassWarSTTask_AttackTargetInstanceData
{
	GENERATED_BODY()

	/** Bind to FMassWarSTEval_FindNearestEnemy's NearestEnemy output. */
	UPROPERTY(EditAnywhere, Category = Input)
	FMassWarUnitHandle Target;
};

/**
 * Writes an Attack order against Target (via FMassWarUnitStateView::RequestAttack, shared with the Actor
 * schema's AttackTargetActor task) and stays Running until MassWarCombat's damage processor (if
 * installed) reverts the order back to Idle - either the target died, or was otherwise invalidated.
 * Doesn't touch health/damage itself, and doesn't depend on MassWarCombat at all: if that plugin isn't
 * installed, this just parks the entity in an Attack order that nothing consumes, harmlessly.
 */
USTRUCT(meta = (DisplayName = "MassWar Attack Target"))
struct MASSWARSTATETREEAI_API FMassWarSTTask_AttackTarget : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FMassWarSTTask_AttackTargetInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;

	TStateTreeExternalDataHandle<FMassWarOrderFragment> OrderHandle;
};
