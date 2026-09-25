// Copyright Epic Games, Inc. All Rights Reserved.

#include "Processors/MassWarDeathProcessor.h"
#include "MassExecutionContext.h"
#include "Fragments/MassWarUnitFragments.h"

UMassWarDeathProcessor::UMassWarDeathProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
}

void UMassWarDeathProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassWarLifeFragment>(EMassFragmentAccess::ReadWrite);
}

void UMassWarDeathProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const float DeltaTime = Context.GetDeltaTimeSeconds();

	EntityQuery.ForEachEntityChunk(Context, [DeltaTime](FMassExecutionContext& Context)
	{
		const TArrayView<FMassWarLifeFragment> LifeList = Context.GetMutableFragmentView<FMassWarLifeFragment>();

		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			FMassWarLifeFragment& Life = LifeList[It];
			if (!Life.IsDying())
			{
				continue;
			}

			Life.TimeDying += DeltaTime;
			if (Life.TimeDying >= Life.LingerTime)
			{
				Context.Defer().DestroyEntity(Context.GetEntity(It));
			}
		}
	});
}
