// Copyright Epic Games, Inc. All Rights Reserved.

#include "Visibility/MassWarVisibilityProcessor.h"
#include "Visibility/MassWarVisibilitySubsystem.h"
#include "Fragments/MassWarVisibilityFragment.h"
#include "Fragments/MassWarUnitFragments.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "Engine/World.h"

UMassWarVisibilityProcessor::UMassWarVisibilityProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);

	// Touches World::GetSubsystem<>() and mutates UMassWarVisibilitySubsystem - see
	// UMassWarLocalVisibilityProcessor's constructor comment for why this can't run off the game thread.
	bRequiresGameThreadExecution = true;
}

void UMassWarVisibilityProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassWarTeamFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassWarVisibilityFragment>(EMassFragmentAccess::ReadOnly);
}

void UMassWarVisibilityProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	TimeSinceLastUpdate += Context.GetDeltaTimeSeconds();
	if (TimeSinceLastUpdate < UpdateInterval)
	{
		return;
	}
	TimeSinceLastUpdate = 0.f;

	UWorld* World = EntityManager.GetWorld();
	UMassWarVisibilitySubsystem* VisibilitySubsystem = World ? World->GetSubsystem<UMassWarVisibilitySubsystem>() : nullptr;
	if (!VisibilitySubsystem)
	{
		return;
	}

	struct FUnitInfo
	{
		FMassEntityHandle Entity;
		uint8 TeamId = 0;
		FVector Location = FVector::ZeroVector;
		float SightRadius = 0.f;
	};

	TArray<FUnitInfo> Units;

	EntityQuery.ForEachEntityChunk(Context, [&Units](FMassExecutionContext& Context)
	{
		const TConstArrayView<FMassWarTeamFragment> TeamList = Context.GetFragmentView<FMassWarTeamFragment>();
		const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FMassWarVisibilityFragment> VisibilityList = Context.GetFragmentView<FMassWarVisibilityFragment>();

		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			FUnitInfo& Unit = Units.AddDefaulted_GetRef();
			Unit.Entity = Context.GetEntity(It);
			Unit.TeamId = TeamList[It].TeamId;
			Unit.Location = TransformList[It].GetTransform().GetLocation();
			Unit.SightRadius = VisibilityList[It].SightRadius;
		}
	});

	TMap<uint8, TSet<FMassEntityHandle>> NewVisibility;

	for (const FUnitInfo& Viewer : Units)
	{
		if (Viewer.TeamId == 0)
		{
			continue;
		}

		TSet<FMassEntityHandle>& VisibleSet = NewVisibility.FindOrAdd(Viewer.TeamId);
		const float SightRadiusSq = FMath::Square(Viewer.SightRadius);

		for (const FUnitInfo& Candidate : Units)
		{
			if (Candidate.TeamId == 0 || Candidate.TeamId == Viewer.TeamId || VisibleSet.Contains(Candidate.Entity))
			{
				continue;
			}

			if (FVector::DistSquared(Viewer.Location, Candidate.Location) <= SightRadiusSq)
			{
				VisibleSet.Add(Candidate.Entity);
			}
		}
	}

	VisibilitySubsystem->SetTeamVisibility(MoveTemp(NewVisibility));
}
