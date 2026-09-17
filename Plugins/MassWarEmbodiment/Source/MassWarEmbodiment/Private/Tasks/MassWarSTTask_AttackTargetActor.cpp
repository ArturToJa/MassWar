// Copyright Epic Games, Inc. All Rights Reserved.

#include "Tasks/MassWarSTTask_AttackTargetActor.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"
#include "Fragments/MassWarUnitFragments.h"
#include "UnitBrain/MassWarUnitStateComponent.h"
#include "UnitBrain/MassWarUnitStateView.h"

bool FMassWarSTTask_AttackTargetActor::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(StateHandle);
	return true;
}

EStateTreeRunStatus FMassWarSTTask_AttackTargetActor::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!InstanceData.Target.IsValid())
	{
		return EStateTreeRunStatus::Failed;
	}

	UMassWarUnitStateComponent& StateComponent = Context.GetExternalData(StateHandle);
	if (IMassWarUnitStateProvider* Provider = Cast<IMassWarUnitStateProvider>(StateComponent.GetOwner()))
	{
		const FMassWarUnitStateView SelfView(*Provider);
		SelfView.RequestAttack(InstanceData.Target);
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FMassWarSTTask_AttackTargetActor::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const UMassWarUnitStateComponent& StateComponent = Context.GetExternalData(StateHandle);
	return StateComponent.Order.OrderType == EMassWarOrderType::Attack ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}
