// Copyright Epic Games, Inc. All Rights Reserved.

#include "Evaluators/MassWarSTEval_PerceivedEnemy.h"
#include "MassStateTreeExecutionContext.h"
#include "MassStateTreeDependency.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"
#include "MassCommonFragments.h"
#include "UnitBrain/MassWarUnitStateView.h"
#include "Engine/World.h"
#include "Formation/MassWarFormationSubsystem.h"
#include "Fragments/MassWarUnitFragments.h"

bool FMassWarSTEval_PerceivedEnemy::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(TransformHandle);
	Linker.LinkExternalData(PerceptionHandle);
	Linker.LinkExternalData(FormationMemberHandle);
	Linker.LinkExternalData(FormationSubsystemHandle);
	return true;
}

void FMassWarSTEval_PerceivedEnemy::GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const
{
	Builder.AddReadOnly<FTransformFragment>();
	Builder.AddReadOnly<FMassWarPerceptionFragment>();
	Builder.AddReadOnly<FMassWarFormationMemberFragment>();
}

void FMassWarSTEval_PerceivedEnemy::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const FMassStateTreeExecutionContext& MassContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	FMassEntityManager& EntityManager = MassContext.GetEntityManager();

	const FMassWarPerceptionFragment& Perception = Context.GetExternalData(PerceptionHandle);
	const FVector SelfLocation = Context.GetExternalData(TransformHandle).GetTransform().GetLocation();
	const UWorld* World = Context.GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;

	InstanceData.bHasEnemy = false;
	InstanceData.bEnemyInSight = false;
	InstanceData.bTookDamageRecently = false;
	for (const FMassWarPerceivedEntry& Entry : Perception.GetEntries())
	{
		if (Entry.WasDamagedBy(Now, InstanceData.RecentDamageWindow))
		{
			InstanceData.bTookDamageRecently = true;
			break;
		}
	}

	// What this unit sees itself first; then what any member of its formation sees (pooled); only then memories.
	TConstArrayView<FMassWarPerceivedEntry> FormationKnowledge;
	if (InstanceData.bUseFormationKnowledge)
	{
		const UMassWarFormationSubsystem& Formations = Context.GetExternalData(FormationSubsystemHandle);
		FormationKnowledge = Formations.GetKnowledge(Context.GetExternalData(FormationMemberHandle).FormationId);
	}

	const FMassWarPerceivedEntry* Best = Perception.FindBest(SelfLocation, Now, /*bIncludeRemembered=*/ false, 0.f);
	if (!Best)
	{
		Best = MassWarFindBestPerceived(FormationKnowledge, SelfLocation, Now, /*bIncludeRemembered=*/ false, 0.f);
	}
	if (!Best && InstanceData.bIncludeRemembered)
	{
		Best = MassWarFindBestPerceived(FormationKnowledge, SelfLocation, Now, true, InstanceData.MaxMemoryAge);
		if (!Best)
		{
			Best = Perception.FindBest(SelfLocation, Now, true, InstanceData.MaxMemoryAge);
		}
	}
	// The memory can lag a moment behind a death; a dead or dying unit is not an enemy to react to.
	if (!Best || !FMassWarUnitStateView::FromHandle(EntityManager, FMassWarUnitHandle(Best->Entity)).IsValid())
	{
		return;
	}

	InstanceData.bHasEnemy = true;
	InstanceData.NearestEnemy = FMassWarUnitHandle(Best->Entity);
	InstanceData.NearestEnemyLocation = FVector(Best->LastKnownLocation);
	InstanceData.DistanceToNearestEnemy = static_cast<float>(FVector::Dist(SelfLocation, InstanceData.NearestEnemyLocation));
	InstanceData.bEnemyInSight = Best->bSeenNow;
	InstanceData.LastSense = Best->LastSense;
	InstanceData.TimeSincePerceived = Best->bSeenNow ? 0.f : static_cast<float>(Best->GetAge(Now));
}
