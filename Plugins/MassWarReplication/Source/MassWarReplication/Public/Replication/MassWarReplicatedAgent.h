// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassClientBubbleHandler.h"
#include "MassReplicationTransformHandlers.h"
#include "MassWarReplicatedAgent.generated.h"

/**
 * Everything MassWarReplication sends over the wire for one unit: position/yaw (via the engine's own
 * position-yaw replication building block) plus Core's team and owning-player ids. Health is
 * deliberately not replicated here - it lives in the optional MassWarCombat plugin, which this plugin
 * (Core-only dependency, per the suite's architecture) does not know about.
 */
USTRUCT()
struct MASSWARREPLICATION_API FReplicatedWarAgent : public FReplicatedAgentBase
{
	GENERATED_BODY()

	FReplicatedWarAgent() = default;

	FReplicatedWarAgent(FMassNetworkID InNetID, FMassEntityTemplateID InTemplateID)
		: FReplicatedAgentBase(InNetID, InTemplateID)
	{
	}

	const FReplicatedAgentPositionYawData& GetReplicatedPositionYawData() const { return PositionYaw; }
	FReplicatedAgentPositionYawData& GetReplicatedPositionYawDataMutable() { return PositionYaw; }

	UPROPERTY(Transient)
	uint8 TeamId = 0;

	UPROPERTY(Transient)
	uint32 OwningPlayerId = 0;

private:
	UPROPERTY(Transient)
	FReplicatedAgentPositionYawData PositionYaw;
};

/** FastArraySerializer item wrapping one FReplicatedWarAgent, per the MassClientBubble convention. */
USTRUCT()
struct MASSWARREPLICATION_API FMassWarFastArrayItem : public FMassFastArrayItemBase
{
	GENERATED_BODY()

	FMassWarFastArrayItem() = default;

	FMassWarFastArrayItem(const FReplicatedWarAgent& InAgent, const FMassReplicatedAgentHandle InHandle)
		: FMassFastArrayItemBase(InHandle)
		, Agent(InAgent)
	{
	}

	typedef FReplicatedWarAgent FReplicatedAgentType;

	UPROPERTY()
	FReplicatedWarAgent Agent;
};
