// Copyright Epic Games, Inc. All Rights Reserved.

#include "Processors/MassWarLoadoutStatsObserver.h"
#include "Fragments/MassWarUnitFragments.h"
#include "Stats/MassWarStatSubsystem.h"
#include "MassExecutionContext.h"
#include "MassEntityView.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassWarLoadoutStatsObserver)

UMassWarLoadoutStatsObserver::UMassWarLoadoutStatsObserver()
	: EntityQuery(*this)
{
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 8
	ObservedTypes = { FMassWarLoadoutFragment::StaticStruct() };
#else
	ObservedType = FMassWarLoadoutFragment::StaticStruct();
#endif
	ObservedOperations = EMassObservedOperationFlags::Add;
	ExecutionFlags = (int32)EProcessorExecutionFlags::AllWorldModes;
	// Stat calculators may be Blueprints, which only run on the game thread.
	bRequiresGameThreadExecution = true;
}

void UMassWarLoadoutStatsObserver::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassWarLoadoutFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassWarStatProfileFragment>(EMassFragmentAccess::ReadWrite);
}

void UMassWarLoadoutStatsObserver::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* World = EntityManager.GetWorld();
	UMassWarStatSubsystem* Stats = World ? World->GetSubsystem<UMassWarStatSubsystem>() : nullptr;
	if (!Stats)
	{
		return;
	}
	EntityQuery.ForEachEntityChunk(Context, [&EntityManager, Stats](FMassExecutionContext& Context)
	{
		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			Stats->RefreshProfile(FMassEntityView(EntityManager, Context.GetEntity(It)));
		}
	});
}