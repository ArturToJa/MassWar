// Copyright Epic Games, Inc. All Rights Reserved.

#include "Processors/MassWarVisualSyncProcessor.h"
#include "Characters/MassWarVisualPuppetInterface.h"
#include "MassActorSubsystem.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "MassRepresentationFragments.h"
#include "Fragments/MassWarUnitFragments.h"

UMassWarVisualSyncProcessor::UMassWarVisualSyncProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	// Same slot the engine's own Mass->Actor translators use: after movement has settled this frame.
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::UpdateWorldFromMass;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);
	// Moves live Actors - game thread only.
	bRequiresGameThreadExecution = true;
}

void UMassWarVisualSyncProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassRepresentationFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassWarLifeFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassWarAttackFeedbackFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.RequireMutatingWorldAccess(); // moves Actors
}

void UMassWarVisualSyncProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const float DeltaTime = Context.GetDeltaTimeSeconds();

	EntityQuery.ForEachEntityChunk(Context, [DeltaTime](FMassExecutionContext& Context)
	{
		const TArrayView<FMassActorFragment> ActorList = Context.GetMutableFragmentView<FMassActorFragment>();
		const TConstArrayView<FMassRepresentationFragment> RepresentationList = Context.GetFragmentView<FMassRepresentationFragment>();
		const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FMassWarLifeFragment> LifeList = Context.GetFragmentView<FMassWarLifeFragment>();
		const TConstArrayView<FMassWarAttackFeedbackFragment> AttackFeedbackList = Context.GetFragmentView<FMassWarAttackFeedbackFragment>();

		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			// Cheap early-out for the vast majority of entities, which are ISM instances / culled.
			const EMassRepresentationType Representation = RepresentationList[It].CurrentRepresentation;
			if (Representation != EMassRepresentationType::HighResSpawnedActor
				&& Representation != EMassRepresentationType::LowResSpawnedActor)
			{
				continue;
			}

			FMassActorFragment& ActorInfo = ActorList[It];
			if (!ActorInfo.IsOwnedByMass())
			{
				continue;
			}

			if (IMassWarVisualPuppet* Puppet = Cast<IMassWarVisualPuppet>(ActorInfo.GetMutable()))
			{
				FMassWarPuppetEntityState EntityState;
				EntityState.bIsDying = LifeList[It].IsDying();
				EntityState.AttackCounter = AttackFeedbackList[It].AttackCounter;
				Puppet->SyncFromEntity(Context.GetEntity(It), TransformList[It].GetTransform(), DeltaTime, EntityState);
			}
		}
	});
}
