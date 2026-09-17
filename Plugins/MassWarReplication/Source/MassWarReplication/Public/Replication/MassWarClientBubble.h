// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassClientBubbleHandler.h"
#include "MassClientBubbleInfoBase.h"
#include "MassReplicationTransformHandlers.h"
#include "Replication/MassWarReplicatedAgent.h"
#include "MassWarClientBubble.generated.h"

/**
 * Per-client handler for MassWar unit replication: owns the transform (position/yaw) sync and writes
 * Core's team id into newly-spawned client-side proxy entities. One instance lives inside
 * FMassWarClientBubbleSerializer, which in turn lives inside AMassWarClientBubbleInfo (one bubble actor
 * per connected client, owned by that client's PlayerController).
 */
class FMassWarClientBubbleHandler : public TClientBubbleHandlerBase<FMassWarFastArrayItem>
{
public:
	typedef TClientBubbleHandlerBase<FMassWarFastArrayItem> Super;
	typedef TMassClientBubbleTransformHandler<FMassWarFastArrayItem> FMassWarTransformHandler;

	FMassWarClientBubbleHandler()
		: TransformHandler(*this)
	{
	}

#if UE_REPLICATION_COMPILE_SERVER_CODE
	FMassWarTransformHandler& GetTransformHandlerMutable() { return TransformHandler; }
#endif // UE_REPLICATION_COMPILE_SERVER_CODE

protected:
#if UE_REPLICATION_COMPILE_CLIENT_CODE
	virtual void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize) override;
	virtual void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize) override;

	void PostReplicatedChangeEntity(const FMassEntityView& EntityView, const FReplicatedWarAgent& Item) const;
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

	FMassWarTransformHandler TransformHandler;
};

/** Fast array of replicated MassWar units for one client, delta-serialized over the network. */
USTRUCT()
struct MASSWARREPLICATION_API FMassWarClientBubbleSerializer : public FMassClientBubbleSerializerBase
{
	GENERATED_BODY()

	FMassWarClientBubbleSerializer()
	{
		Bubble.Initialize(WarAgents, *this);
	}

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FMassWarFastArrayItem, FMassWarClientBubbleSerializer>(WarAgents, DeltaParams, *this);
	}

	FMassWarClientBubbleHandler Bubble;

protected:
	/** Freelist-ordered on the server; array order is not guaranteed to match between server and client - FMassNetworkID is what's stable. */
	UPROPERTY(Transient)
	TArray<FMassWarFastArrayItem> WarAgents;
};

template<>
struct TStructOpsTypeTraits<FMassWarClientBubbleSerializer> : public TStructOpsTypeTraitsBase2<FMassWarClientBubbleSerializer>
{
	enum
	{
		WithNetDeltaSerializer = true,
		WithCopy = false,
	};
};

/** One spawned per connected client (owned by that client's PlayerController), carries the replicated fast array. */
UCLASS()
class MASSWARREPLICATION_API AMassWarClientBubbleInfo : public AMassClientBubbleInfoBase
{
	GENERATED_BODY()

public:
	AMassWarClientBubbleInfo(const FObjectInitializer& ObjectInitializer);

	FMassWarClientBubbleSerializer& GetWarSerializer() { return WarSerializer; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, Transient)
	FMassWarClientBubbleSerializer WarSerializer;
};
