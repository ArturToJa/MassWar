// Copyright Epic Games, Inc. All Rights Reserved.

#include "Visibility/MassWarVisibilitySubsystem.h"
#include "Subsystem/MassWarReplicationSetupSubsystem.h"
#include "Interfaces/MassWarTeamProviderInterface.h"
#include "Fragments/MassWarUnitFragments.h"
#include "MassSpawnerSubsystem.h"
#include "Visibility/MassWarGhostSubsystem.h"
#include "UnitBrain/MassWarUnitStateView.h"
#include "MassCommonFragments.h"
#include "MassEntityManager.h"
#include "GameFramework/PlayerController.h"

void UMassWarVisibilitySubsystem::PostInitialize()
{
	Super::PostInitialize();

	if (UMassWarReplicationSetupSubsystem* ReplicationSetup = GetWorld()->GetSubsystem<UMassWarReplicationSetupSubsystem>())
	{
		ReplicationSetup->OnFilterRelevancy.BindUObject(this, &UMassWarVisibilitySubsystem::IsRelevant);
	}
}

void UMassWarVisibilitySubsystem::SetTeamVisibility(TMap<uint8, TSet<FMassEntityHandle>>&& InTeamVisibleEnemies)
{
	// The local player of a listen server / standalone game has no replication bubble to lose units from, so its
	// ghosts come from here: a unit its team could see and now cannot leaves a ghost; one it sees again clears it.
	UWorld* World = GetWorld();
	UMassWarGhostSubsystem* Ghosts = (World && !World->IsNetMode(NM_Client)) ? World->GetSubsystem<UMassWarGhostSubsystem>() : nullptr;
	UMassSpawnerSubsystem* Spawner = Ghosts ? World->GetSubsystem<UMassSpawnerSubsystem>() : nullptr;
	const uint8 LocalTeamId = Ghosts ? Ghosts->GetLocalTeamId() : 0;
	if (Spawner && LocalTeamId != 0)
	{
		FMassEntityManager& EntityManager = Spawner->GetEntityManagerChecked();
		const TSet<FMassEntityHandle>* OldVisible = TeamVisibleEnemies.Find(LocalTeamId);
		const TSet<FMassEntityHandle>* NewVisible = InTeamVisibleEnemies.Find(LocalTeamId);
		if (OldVisible)
		{
			for (const FMassEntityHandle& Entity : *OldVisible)
			{
				if (NewVisible && NewVisible->Contains(Entity))
				{
					continue;
				}
				const FTransformFragment* Transform = FMassWarUnitStateView::IsLiving(EntityManager, Entity) ? EntityManager.GetFragmentDataPtr<FTransformFragment>(Entity) : nullptr;
				const FMassWarTeamFragment* Team = Transform ? EntityManager.GetFragmentDataPtr<FMassWarTeamFragment>(Entity) : nullptr;
				if (Transform && Team)
				{
					Ghosts->AddGhostForEntity(Entity, Transform->GetTransform().GetLocation(), Transform->GetTransform().Rotator().Yaw, Team->TeamId);
				}
			}
		}
		if (NewVisible)
		{
			for (const FMassEntityHandle& Entity : *NewVisible)
			{
				if (!OldVisible || !OldVisible->Contains(Entity))
				{
					Ghosts->ClearGhostForEntity(Entity);
				}
			}
		}
	}

	TeamVisibleEnemies = MoveTemp(InTeamVisibleEnemies);
}

bool UMassWarVisibilitySubsystem::IsVisibleToTeam(FMassEntityHandle Entity, uint8 TeamId) const
{
	if (const TSet<FMassEntityHandle>* VisibleSet = TeamVisibleEnemies.Find(TeamId))
	{
		return VisibleSet->Contains(Entity);
	}
	return false;
}

bool UMassWarVisibilitySubsystem::IsRelevant(FMassEntityHandle Entity, APlayerController* ViewerController) const
{
	const IMassWarTeamProvider* TeamProvider = Cast<IMassWarTeamProvider>(ViewerController);
	if (!TeamProvider)
	{
		// Not a MassWar player controller (e.g. a spectator) - fail open rather than hide everything.
		return true;
	}

	const uint8 ViewerTeamId = TeamProvider->GetMassWarPlayerTeamId();

	UMassSpawnerSubsystem* Spawner = GetWorld() ? GetWorld()->GetSubsystem<UMassSpawnerSubsystem>() : nullptr;
	if (!Spawner)
	{
		return true;
	}

	const FMassEntityManager& EntityManager = Spawner->GetEntityManagerChecked();
	const FMassWarTeamFragment* EntityTeam = EntityManager.GetFragmentDataPtr<FMassWarTeamFragment>(Entity);
	if (!EntityTeam || EntityTeam->TeamId == 0 || EntityTeam->TeamId == ViewerTeamId)
	{
		// Own team (and neutral) is always visible - fog of war only ever hides enemies.
		return true;
	}

	return IsVisibleToTeam(Entity, ViewerTeamId);
}
