// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"
#include "MassWarTeamProviderInterface.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UMassWarTeamProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by whatever represents a player's identity (their PlayerController) so Core-dependent-only
 * plugins can learn which team a viewer plays without depending on MassWarSelection (which owns
 * AMassWarSelectionPlayerController::PlayerTeamId) - e.g. MassWarFogOfWar casts a replication bubble's
 * owning controller to this interface to filter relevancy per team, staying within its documented
 * "Core, MassWarReplication" dependency list.
 */
class MASSWAR_API IMassWarTeamProvider
{
	GENERATED_BODY()

public:
	virtual uint8 GetMassWarPlayerTeamId() const = 0;
};
