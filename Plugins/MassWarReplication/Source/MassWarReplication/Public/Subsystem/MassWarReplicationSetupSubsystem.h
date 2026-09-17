// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "MassEntityHandle.h"
#include "MassWarReplicationSetupSubsystem.generated.h"

class APlayerController;

/**
 * Per-(entity, client) relevancy filter. Unbound (default) means "always relevant" - matches behavior
 * before any filtering plugin is installed. Called once per candidate entity, per client, per
 * replication tick, only for entities already within normal replication distance; returning false stops
 * that entity from ever being replicated to that client (or removes it immediately if it already was).
 * MassWarReplication itself has no idea what's bound here (e.g. MassWarFogOfWar's team/visibility
 * logic) - see UMassWarReplicationSetupSubsystem::OnFilterRelevancy.
 */
DECLARE_DELEGATE_RetVal_TwoParams(bool, FMassWarReplicationRelevancyDelegate, FMassEntityHandle /*Entity*/, APlayerController* /*ViewerController*/);

/**
 * Registers AMassWarClientBubbleInfo with UMassReplicationSubsystem before any client connects.
 * PostInitialize() (called after every world subsystem's own Initialize()) guarantees
 * UMassReplicationSubsystem already exists, and RegisterBubbleInfoClass() must run before its
 * AddClient()/SynchronizeClientsAndViewers() - i.e. before PIE actually starts ticking.
 */
UCLASS()
class MASSWARREPLICATION_API UMassWarReplicationSetupSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Bind via GetWorld()->GetSubsystem<UMassWarReplicationSetupSubsystem>()->OnFilterRelevancy.BindUObject(...). */
	FMassWarReplicationRelevancyDelegate OnFilterRelevancy;

protected:
	virtual void PostInitialize() override;
};
