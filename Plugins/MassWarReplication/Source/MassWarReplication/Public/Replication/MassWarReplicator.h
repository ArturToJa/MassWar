// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassReplicationProcessor.h"
#include "MassWarReplicator.generated.h"

/** Server-only: drives MassWarClientBubble from Core's fragments (transform, team) each replication tick. */
UCLASS()
class MASSWARREPLICATION_API UMassWarReplicator : public UMassReplicatorBase
{
	GENERATED_BODY()

public:
	virtual void AddRequirements(FMassEntityQuery& EntityQuery) override;
	virtual void ProcessClientReplication(FMassExecutionContext& Context, FMassReplicationContext& ReplicationContext) override;
};
