// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "MassEntityHandle.h"
#include "MassWarVisibilitySubsystem.generated.h"

class APlayerController;

/**
 * Server-only store of "which enemy entities is each team currently able to see" (radius-only for now -
 * a line-of-sight raycast pass is a documented follow-up), refreshed periodically by
 * UMassWarVisibilityProcessor. Binds MassWarReplication's relevancy delegate in PostInitialize() so an
 * enemy entity outside a team's sight is never replicated to that team's clients - Replication has no
 * idea this plugin exists; this is the one piece of code that wires the two together.
 */
UCLASS()
class MASSWARFOGOFWAR_API UMassWarVisibilitySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Replaces the whole visibility snapshot; called once per update by UMassWarVisibilityProcessor. */
	void SetTeamVisibility(TMap<uint8, TSet<FMassEntityHandle>>&& InTeamVisibleEnemies);

	bool IsVisibleToTeam(FMassEntityHandle Entity, uint8 TeamId) const;

protected:
	virtual void PostInitialize() override;

private:
	/** Bound to MassWarReplicationSetupSubsystem::OnFilterRelevancy. */
	bool IsRelevant(FMassEntityHandle Entity, APlayerController* ViewerController) const;

	TMap<uint8, TSet<FMassEntityHandle>> TeamVisibleEnemies;
};
