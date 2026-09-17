// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityHandle.h"
#include "MassWarOrderTarget.generated.h"

/**
 * One unit as sent over UMassWarUnitOrderComponent's order RPCs. Carries both the sending client's own
 * local FMassEntityHandle and Core's cross-network NetId (0 if MassWarReplication isn't installed):
 * a listen-server's own local player has a Handle that's already valid on the server directly (same
 * Mass world, zero translation, zero Replication dependency - exactly how order issuing worked before
 * this struct existed); a remote client's Handle is only valid in that client's own local replicated
 * Mass world, so the server falls back to resolving NetId via UMassWarUnitRegistrySubsystem::FindByNetId.
 */
USTRUCT()
struct FMassWarOrderTarget
{
	GENERATED_BODY()

	UPROPERTY()
	FMassEntityHandle Handle;

	UPROPERTY()
	uint32 NetId = 0;
};
