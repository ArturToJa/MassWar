// Copyright Epic Games, Inc. All Rights Reserved.

#include "Evaluators/MassWarSTEval_FindNearestEnemy.h"
#include "MassStateTreeExecutionContext.h"
#include "MassStateTreeDependency.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"
#include "MassCommonFragments.h"
#include "Fragments/MassWarUnitFragments.h"
#include "Registry/MassWarUnitRegistrySubsystem.h"
#include "UnitBrain/MassWarUnitStateView.h"

bool FMassWarSTEval_FindNearestEnemy::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(TransformHandle);
	Linker.LinkExternalData(TeamHandle);
	Linker.LinkExternalData(RegistryHandle);
	return true;
}

void FMassWarSTEval_FindNearestEnemy::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadOnly<FTransformFragment>();
	Builder.AddReadOnly<FMassWarTeamFragment>();
}

void FMassWarSTEval_FindNearestEnemy::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const FMassStateTreeExecutionContext& MassContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	FMassEntityManager& EntityManager = MassContext.GetEntityManager();

	const FMassWarTeamFragment& SelfTeam = Context.GetExternalData(TeamHandle);
	const UMassWarUnitRegistrySubsystem& Registry = Context.GetExternalData(RegistryHandle);

	const FMassWarUnitStateView SelfView(EntityManager, MassContext.GetEntity());
	const FMassWarUnitHandle SelfHandle(MassContext.GetEntity());

	const FMassWarNearestEnemyResult Result = FMassWarUnitStateView::FindNearestEnemy(
		EntityManager, SelfTeam.TeamId, SelfView.GetLocation(), SelfHandle, Registry.GetAllUnitHandles(), InstanceData.SearchRadius);

	InstanceData.bHasEnemy = Result.bHasEnemy;
	InstanceData.NearestEnemy = Result.NearestEnemy;
	InstanceData.NearestEnemyLocation = Result.NearestEnemyLocation;
	InstanceData.DistanceToNearestEnemy = Result.DistanceToNearestEnemy;
}
