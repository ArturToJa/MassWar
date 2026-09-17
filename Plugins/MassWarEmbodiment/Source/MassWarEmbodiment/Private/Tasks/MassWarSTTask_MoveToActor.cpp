// Copyright Epic Games, Inc. All Rights Reserved.

#include "Tasks/MassWarSTTask_MoveToActor.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"
#include "Fragments/MassWarUnitFragments.h"
#include "UnitBrain/MassWarUnitStateComponent.h"
#include "UnitBrain/MassWarUnitStateView.h"

bool FMassWarSTTask_MoveToActor::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(StateHandle);
	return true;
}

EStateTreeRunStatus FMassWarSTTask_MoveToActor::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	UMassWarUnitStateComponent& StateComponent = Context.GetExternalData(StateHandle);

	if (IMassWarUnitStateProvider* Provider = Cast<IMassWarUnitStateProvider>(StateComponent.GetOwner()))
	{
		const FMassWarUnitStateView SelfView(*Provider);
		SelfView.RequestMoveTo(InstanceData.Destination);
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FMassWarSTTask_MoveToActor::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const UMassWarUnitStateComponent& StateComponent = Context.GetExternalData(StateHandle);
	return StateComponent.Order.OrderType == EMassWarOrderType::Move ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}
