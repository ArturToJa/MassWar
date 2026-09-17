// Copyright Epic Games, Inc. All Rights Reserved.

#include "Conditions/MassWarSTCondition_HasEnemyInRange.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "MassStateTreeDependency.h"
#include "Fragments/MassWarUnitFragments.h"

bool FMassWarSTCondition_HasEnemyInRange::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(OrderHandle);
	return true;
}

void FMassWarSTCondition_HasEnemyInRange::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadOnly<FMassWarOrderFragment>();
}

bool FMassWarSTCondition_HasEnemyInRange::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const FMassWarOrderFragment& Order = Context.GetExternalData(OrderHandle);

	if (Order.bPlayerCommanded)
	{
		return false;
	}

	return InstanceData.bHasEnemy && InstanceData.DistanceToEnemy <= InstanceData.Range;
}
