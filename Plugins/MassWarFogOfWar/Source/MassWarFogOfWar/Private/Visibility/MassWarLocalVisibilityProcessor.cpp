// Copyright Epic Games, Inc. All Rights Reserved.

#include "Visibility/MassWarLocalVisibilityProcessor.h"
#include "Visibility/MassWarVisibilitySubsystem.h"
#include "Fragments/MassWarUnitFragments.h"
#include "Interfaces/MassWarTeamProviderInterface.h"
#include "MassRepresentationFragments.h"
#include "MassLODTypes.h"
#include "MassRepresentationTypes.h"
#include "MassExecutionContext.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

UMassWarLocalVisibilityProcessor::UMassWarLocalVisibilityProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);

	// Touches World::GetPlayerControllerIterator/Cast<>/GetSubsystem<>, none of which are safe to call
	// off the game thread - Mass is otherwise free to schedule this on a worker thread in parallel with
	// game-thread Actor spawn/destroy (e.g. MassWarEmbodiment's near-LOD actor swap, which churns heavily
	// exactly when units cluster up in combat), which crashed intermittently reading a subsystem pointer
	// mid-mutation.
	bRequiresGameThreadExecution = true;

	// Must run after the LOD group has computed its normal distance-based FMassRepresentationLODFragment,
	// and before the Representation group reads it to decide what actually gets rendered this frame.
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::LOD);
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::Representation);
}

void UMassWarLocalVisibilityProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassWarTeamFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadWrite);
}

void UMassWarLocalVisibilityProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* World = EntityManager.GetWorld();
	if (!World)
	{
		return;
	}

	// At most one PlayerController is ever local to this process - none on a dedicated server (no-op),
	// exactly one on a listen server (the host) or standalone game.
	const IMassWarTeamProvider* LocalTeamProvider = nullptr;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		if (PC && PC->IsLocalController())
		{
			LocalTeamProvider = Cast<IMassWarTeamProvider>(PC);
			break;
		}
	}

	if (!LocalTeamProvider)
	{
		return;
	}

	const uint8 LocalTeamId = LocalTeamProvider->GetMassWarPlayerTeamId();

	UMassWarVisibilitySubsystem* VisibilitySubsystem = World->GetSubsystem<UMassWarVisibilitySubsystem>();
	if (!VisibilitySubsystem)
	{
		return;
	}

	EntityQuery.ForEachEntityChunk(Context, [VisibilitySubsystem, LocalTeamId](FMassExecutionContext& Context)
	{
		const TConstArrayView<FMassWarTeamFragment> TeamList = Context.GetFragmentView<FMassWarTeamFragment>();
		const TArrayView<FMassRepresentationLODFragment> RepLODList = Context.GetMutableFragmentView<FMassRepresentationLODFragment>();

		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			const uint8 EntityTeamId = TeamList[It].TeamId;
			if (EntityTeamId == 0 || EntityTeamId == LocalTeamId)
			{
				// Own team (and neutral) always renders locally - fog of war only ever hides enemies.
				continue;
			}

			if (!VisibilitySubsystem->IsVisibleToTeam(Context.GetEntity(It), LocalTeamId))
			{
				RepLODList[It].LOD = EMassLOD::Off;
			}
		}
	});
}
