// Copyright Epic Games, Inc. All Rights Reserved.

#include "Tasks/MassWarSTTask_MoveTo.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"
#include "MassStateTreeDependency.h"
#include "MassStateTreeExecutionContext.h"
#include "Fragments/MassWarUnitFragments.h"
#include "UnitBrain/MassWarUnitStateView.h"

bool FMassWarSTTask_MoveTo::Link(FStateTreeLinker& Linker)
{
	// OrderHandle isn't read directly below (FMassWarUnitStateView does its own EntityManager lookup),
	// but the link is still required so Mass's query/dependency system guarantees this entity actually
	// has FMassWarOrderFragment before this task ever ticks.
	Linker.LinkExternalData(OrderHandle);
	return true;
}

void FMassWarSTTask_MoveTo::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadWrite<FMassWarOrderFragment>();
}

EStateTreeRunStatus FMassWarSTTask_MoveTo::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const FMassStateTreeExecutionContext& MassContext = static_cast<FMassStateTreeExecutionContext&>(Context);

	const FMassWarUnitStateView SelfView(MassContext.GetEntityManager(), MassContext.GetEntity());
	SelfView.RequestMoveTo(InstanceData.Destination);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FMassWarSTTask_MoveTo::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FMassWarOrderFragment& Order = Context.GetExternalData(OrderHandle);
	return Order.OrderType == EMassWarOrderType::Move ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}
