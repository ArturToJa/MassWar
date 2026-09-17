// Copyright Epic Games, Inc. All Rights Reserved.

#include "Visibility/MassWarVisibilitySubsystem.h"
#include "Subsystem/MassWarReplicationSetupSubsystem.h"
#include "Interfaces/MassWarTeamProviderInterface.h"
#include "Fragments/MassWarUnitFragments.h"
#include "MassSpawnerSubsystem.h"
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
