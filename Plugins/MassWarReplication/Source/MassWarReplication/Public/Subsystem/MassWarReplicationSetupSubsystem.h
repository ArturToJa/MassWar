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

/** NetId = the unit's replicated id (FMassWarNetIdFragment); Location/YawDegrees = its last replicated pose; LifeState = EMassWarLifeState. */
DECLARE_MULTICAST_DELEGATE_FiveParams(FMassWarClientAgentRemovedDelegate, uint32 /*NetId*/, const FVector& /*Location*/, float /*YawDegrees*/, uint8 /*TeamId*/, uint8 /*LifeState*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FMassWarClientAgentAddedDelegate, uint32 /*NetId*/);

/**
 * Registers AMassWarClientBubbleInfo with UMassReplicationSubsystem before any client connects.
 * Done in Initialize() (declaring UMassReplicationSubsystem as a dependency, so it exists), NOT PostInitialize():
 * anything that builds a replicated entity template needs the class registered first - e.g. a game subsystem that
 * builds its unit templates in its own PostInitialize() - and every subsystem's Initialize() completes before any
 * PostInitialize() starts, whereas the order of two PostInitialize() calls is not defined (it differed between a
 * PIE session and a standalone run, leaving templates with an invalid bubble class handle).
 * RegisterBubbleInfoClass() must also run before AddClient()/SynchronizeClientsAndViewers().
 */
UCLASS()
class MASSWARREPLICATION_API UMassWarReplicationSetupSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Bind via GetWorld()->GetSubsystem<UMassWarReplicationSetupSubsystem>()->OnFilterRelevancy.BindUObject(...). */
	FMassWarReplicationRelevancyDelegate OnFilterRelevancy;

	/** CLIENT side: a replicated unit just left this client's bubble (out of relevancy or destroyed on the server).
	 *  Fired just before its client-side entity is destroyed, with the last replicated state - so an optional
	 *  plugin (MassWarFogOfWar's ghosts) can remember where it was without Replication knowing it exists. */
	FMassWarClientAgentRemovedDelegate OnClientAgentRemoved;

	/** CLIENT side: a replicated unit entered (or re-entered) this client's bubble. */
	FMassWarClientAgentAddedDelegate OnClientAgentAdded;

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
};
