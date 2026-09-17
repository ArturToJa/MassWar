// Copyright Epic Games, Inc. All Rights Reserved.

#include "Evaluators/MassWarSTEval_FindNearestEnemyActor.h"
#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "MassSpawnerSubsystem.h"
#include "Registry/MassWarUnitRegistrySubsystem.h"
#include "UnitBrain/MassWarUnitStateComponent.h"
#include "UnitBrain/MassWarUnitStateView.h"

bool FMassWarSTEval_FindNearestEnemyActor::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(StateHandle);
	Linker.LinkExternalData(RegistryHandle);
	Linker.LinkExternalData(SpawnerHandle);
	return true;
}

void FMassWarSTEval_FindNearestEnemyActor::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	UMassWarUnitStateComponent& StateComponent = Context.GetExternalData(StateHandle);
	const UMassWarUnitRegistrySubsystem& Registry = Context.GetExternalData(RegistryHandle);
	UMassSpawnerSubsystem& Spawner = Context.GetExternalData(SpawnerHandle);

	AActor* SelfActor = StateComponent.GetOwner();
	IMassWarUnitStateProvider* Provider = Cast<IMassWarUnitStateProvider>(SelfActor);
	if (!Provider)
	{
		return;
	}

	FMassEntityManager& EntityManager = Spawner.GetEntityManagerChecked();

	const FMassWarUnitStateView SelfView(*Provider);
	const FMassWarUnitHandle SelfHandle(SelfActor);

	const FMassWarNearestEnemyResult Result = FMassWarUnitStateView::FindNearestEnemy(
		EntityManager, SelfView.GetTeam(), SelfView.GetLocation(), SelfHandle, Registry.GetAllUnitHandles(), InstanceData.SearchRadius);

	InstanceData.bHasEnemy = Result.bHasEnemy;
	InstanceData.NearestEnemy = Result.NearestEnemy;
	InstanceData.NearestEnemyLocation = Result.NearestEnemyLocation;
	InstanceData.DistanceToNearestEnemy = Result.DistanceToNearestEnemy;
}
