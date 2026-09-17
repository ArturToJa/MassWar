// Copyright Epic Games, Inc. All Rights Reserved.

#include "Tasks/MassWarSTTask_AttackTarget.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"
#include "MassStateTreeDependency.h"
#include "MassStateTreeExecutionContext.h"
#include "Fragments/MassWarUnitFragments.h"
#include "UnitBrain/MassWarUnitStateView.h"

bool FMassWarSTTask_AttackTarget::Link(FStateTreeLinker& Linker)
{
	// OrderHandle isn't read directly below (FMassWarUnitStateView does its own EntityManager lookup),
	// but the link is still required so Mass's query/dependency system guarantees this entity actually
	// has FMassWarOrderFragment before this task ever ticks.
	Linker.LinkExternalData(OrderHandle);
	return true;
}

void FMassWarSTTask_AttackTarget::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadWrite<FMassWarOrderFragment>();
}

EStateTreeRunStatus FMassWarSTTask_AttackTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const FMassStateTreeExecutionContext& MassContext = static_cast<FMassStateTreeExecutionContext&>(Context);

	if (!InstanceData.Target.IsValid())
	{
		return EStateTreeRunStatus::Failed;
	}

	const FMassWarUnitStateView SelfView(MassContext.GetEntityManager(), MassContext.GetEntity());
	SelfView.RequestAttack(InstanceData.Target);

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FMassWarSTTask_AttackTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FMassWarOrderFragment& Order = Context.GetExternalData(OrderHandle);
	return Order.OrderType == EMassWarOrderType::Attack ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}
