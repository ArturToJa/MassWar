// Copyright Epic Games, Inc. All Rights Reserved.

#include "Processors/MassWarSTHeartbeatProcessor.h"
#include "MassExecutionContext.h"
#include "MassSignalSubsystem.h"
#include "MassStateTreeFragments.h"
#include "MassStateTreeTypes.h"

UMassWarSTHeartbeatProcessor::UMassWarSTHeartbeatProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
}

void UMassWarSTHeartbeatProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassStateTreeInstanceFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddSubsystemRequirement<UMassSignalSubsystem>(EMassFragmentAccess::ReadWrite);

	ProcessorRequirements.AddSubsystemRequirement<UMassSignalSubsystem>(EMassFragmentAccess::ReadWrite);
}

void UMassWarSTHeartbeatProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	TimeSinceLastSignal += Context.GetDeltaTimeSeconds();
	if (TimeSinceLastSignal < SignalIntervalSeconds)
	{
		return;
	}
	TimeSinceLastSignal = 0.f;

	TArray<FMassEntityHandle> EntitiesToSignal;
	EntityQuery.ForEachEntityChunk(Context, [&EntitiesToSignal](FMassExecutionContext& ChunkContext)
	{
		for (FMassExecutionContext::FEntityIterator It = ChunkContext.CreateEntityIterator(); It; ++It)
		{
			EntitiesToSignal.Add(ChunkContext.GetEntity(It));
		}
	});

	if (EntitiesToSignal.Num() > 0)
	{
		UMassSignalSubsystem& SignalSubsystem = Context.GetMutableSubsystemChecked<UMassSignalSubsystem>();
		SignalSubsystem.SignalEntities(UE::Mass::Signals::NewStateTreeTaskRequired, EntitiesToSignal);
	}
}
