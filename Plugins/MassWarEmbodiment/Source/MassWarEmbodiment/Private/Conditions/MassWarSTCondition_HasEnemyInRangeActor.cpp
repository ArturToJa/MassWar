// Copyright Epic Games, Inc. All Rights Reserved.

#include "Conditions/MassWarSTCondition_HasEnemyInRangeActor.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "Fragments/MassWarUnitFragments.h"
#include "UnitBrain/MassWarUnitStateComponent.h"

bool FMassWarSTCondition_HasEnemyInRangeActor::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(StateHandle);
	return true;
}

bool FMassWarSTCondition_HasEnemyInRangeActor::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const UMassWarUnitStateComponent& StateComponent = Context.GetExternalData(StateHandle);

	if (StateComponent.Order.bPlayerCommanded)
	{
		return false;
	}

	return InstanceData.bHasEnemy && InstanceData.DistanceToEnemy <= InstanceData.Range;
}
