// Copyright Epic Games, Inc. All Rights Reserved.

#include "Processors/MassWarPerceptionDebugProcessor.h"
#include "Processors/MassWarPerceptionProcessor.h"
#include "Fragments/MassWarPerceptionFragments.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarPerceptionDebug(
		TEXT("MassWar.Perception.Debug"), 0,
		TEXT("1 = draw what MassWar units perceive (green sight, yellow hearing, red damage, grey remembered)."),
		ECVF_Default);

	constexpr int32 MaxLinesPerFrame = 1500;
}

UMassWarPerceptionDebugProcessor::UMassWarPerceptionDebugProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
	bRequiresGameThreadExecution = true; // debug drawing is a game-thread API
	ExecutionOrder.ExecuteAfter.Add(UMassWarPerceptionProcessor::StaticClass()->GetFName());
}

void UMassWarPerceptionDebugProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassWarPerceptionFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
}

void UMassWarPerceptionDebugProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* World = GetWorld();
	if (!World || CVarPerceptionDebug.GetValueOnAnyThread() == 0)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	int32 Lines = 0;

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TConstArrayView<FMassWarPerceptionFragment> PerceptionList = Context.GetFragmentView<FMassWarPerceptionFragment>();
		const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();

		for (int32 Index = 0; Index < Context.GetNumEntities() && Lines < MaxLinesPerFrame; ++Index)
		{
			const FVector From = TransformList[Index].GetTransform().GetLocation() + FVector(0.0, 0.0, 120.0);
			for (const FMassWarPerceivedEntry& Entry : PerceptionList[Index].GetEntries())
			{
				FColor Color = FColor(110, 110, 110);
				if (Entry.bSeenNow)
				{
					Color = FColor::Green;
				}
				else if (Entry.GetAge(Now) < 1.0)
				{
					Color = Entry.LastSense == EMassWarSense::Damage ? FColor::Red : (Entry.LastSense == EMassWarSense::Hearing ? FColor::Yellow : Color);
				}
				const FVector To = FVector(Entry.LastKnownLocation) + FVector(0.0, 0.0, 120.0);
				DrawDebugLine(World, From, To, Color, false, 0.f, 0, 2.f);
				DrawDebugPoint(World, To, 12.f, Color, false, 0.f);
				++Lines;
			}
		}
	});
}
