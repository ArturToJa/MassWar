// Copyright Epic Games, Inc. All Rights Reserved.

#include "Processors/MassWarDyingVisibilityProcessor.h"
#include "Fragments/MassWarUnitFragments.h"
#include "MassExecutionContext.h"
#include "MassLODTypes.h"
#include "MassRepresentationFragments.h"
#include "MassRepresentationTypes.h"

UMassWarDyingVisibilityProcessor::UMassWarDyingVisibilityProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllNetModes;
	// Between Mass computing a unit's visualization LOD and Mass acting on it (deliberately not inside
	// either group - a processor can't be ordered after the group it belongs to).
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::LOD);
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Representation);
}

void UMassWarDyingVisibilityProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassWarLifeFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadWrite);
}

void UMassWarDyingVisibilityProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		const TConstArrayView<FMassWarLifeFragment> LifeList = Context.GetFragmentView<FMassWarLifeFragment>();
		const TArrayView<FMassRepresentationLODFragment> LODList = Context.GetMutableFragmentView<FMassRepresentationLODFragment>();

		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			if (LifeList[It].IsDying() && LODList[It].LOD != EMassLOD::High)
			{
				LODList[It].LOD = EMassLOD::Off;
			}
		}
	});
}
