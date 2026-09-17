// Copyright Epic Games, Inc. All Rights Reserved.

#include "Replication/MassWarReplicator.h"
#include "Replication/MassWarClientBubble.h"
#include "Fragments/MassWarUnitFragments.h"
#include "Subsystem/MassWarReplicationSetupSubsystem.h"
#include "MassExecutionContext.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassWarReplicator)

void UMassWarReplicator::AddRequirements(FMassEntityQuery& EntityQuery)
{
	FMassReplicationProcessorPositionYawHandler::AddRequirements(EntityQuery);
	EntityQuery.AddRequirement<FMassWarTeamFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassWarOwnerFragment>(EMassFragmentAccess::ReadOnly);
	// ReadWrite: mirrors the engine-assigned FMassNetworkID into Core's own fragment (see AddEntityCallback
	// below) so Core-only-dependent plugins like MassWarSelection can resolve units cross-network without
	// depending on MassWarReplication themselves.
	EntityQuery.AddRequirement<FMassWarNetIdFragment>(EMassFragmentAccess::ReadWrite);
}

void UMassWarReplicator::ProcessClientReplication(FMassExecutionContext& Context, FMassReplicationContext& ReplicationContext)
{
#if UE_REPLICATION_COMPILE_SERVER_CODE
	FMassReplicationProcessorPositionYawHandler PositionYawHandler;
	TConstArrayView<FMassWarTeamFragment> TeamList;
	TConstArrayView<FMassWarOwnerFragment> OwnerList;
	TArrayView<FMassWarNetIdFragment> NetIdList;
	FMassReplicationSharedFragment* RepSharedFrag = nullptr;

	auto CacheViewsCallback = [&](FMassExecutionContext& Context)
	{
		PositionYawHandler.CacheFragmentViews(Context);
		TeamList = Context.GetFragmentView<FMassWarTeamFragment>();
		OwnerList = Context.GetFragmentView<FMassWarOwnerFragment>();
		NetIdList = Context.GetMutableFragmentView<FMassWarNetIdFragment>();
		RepSharedFrag = &Context.GetMutableSharedFragment<FMassReplicationSharedFragment>();
		check(RepSharedFrag);

		// Optional relevancy filter (e.g. MassWarFogOfWar) - forcing LOD to Off here, before the base
		// class's own Add/Modify/Remove dispatch reads it below, makes a filtered-out entity behave
		// exactly like one that's out of replication distance: never added while hidden, and immediately
		// removed the moment it stops being relevant. Left unbound (no filtering plugin installed), every
		// entity keeps whatever LOD the normal distance-based calculation already gave it.
		UMassWarReplicationSetupSubsystem* Setup = UWorld::GetSubsystem<UMassWarReplicationSetupSubsystem>(&ReplicationContext.World);
		if (Setup && Setup->OnFilterRelevancy.IsBound())
		{
			AMassWarClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<AMassWarClientBubbleInfo>(RepSharedFrag->CurrentClientHandle);
			APlayerController* ViewerController = Cast<APlayerController>(BubbleInfo.GetOwner());

			const TArrayView<FMassReplicationLODFragment> LODList = Context.GetMutableFragmentView<FMassReplicationLODFragment>();
			for (int32 Idx = 0; Idx < TeamList.Num(); ++Idx)
			{
				if (LODList[Idx].LOD < EMassLOD::Off && !Setup->OnFilterRelevancy.Execute(Context.GetEntity(Idx), ViewerController))
				{
					LODList[Idx].LOD = EMassLOD::Off;
				}
			}
		}
	};

	auto AddEntityCallback = [&](FMassExecutionContext& Context, const int32 EntityIdx, FReplicatedWarAgent& InReplicatedAgent, const FMassClientHandle ClientHandle) -> FMassReplicatedAgentHandle
	{
		AMassWarClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<AMassWarClientBubbleInfo>(ClientHandle);

		PositionYawHandler.AddEntity(EntityIdx, InReplicatedAgent.GetReplicatedPositionYawDataMutable());
		InReplicatedAgent.TeamId = TeamList[EntityIdx].TeamId;
		InReplicatedAgent.OwningPlayerId = OwnerList[EntityIdx].OwningPlayerId;

		// The engine already assigned InReplicatedAgent's NetID (via FMassNetworkIDFragment) before this
		// callback runs - mirror it into Core's own fragment so Selection/Registry can resolve this unit
		// by NetId without depending on MassWarReplication. Redundant-but-harmless if multiple clients
		// are connected (same value written each time).
		NetIdList[EntityIdx].NetId = InReplicatedAgent.GetNetID().GetValue();

		return BubbleInfo.GetWarSerializer().Bubble.AddAgent(Context.GetEntity(EntityIdx), InReplicatedAgent);
	};

	auto ModifyEntityCallback = [&](FMassExecutionContext& Context, const int32 EntityIdx, const EMassLOD::Type LOD, const double Time, const FMassReplicatedAgentHandle Handle, const FMassClientHandle ClientHandle)
	{
		AMassWarClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<AMassWarClientBubbleInfo>(ClientHandle);
		FMassWarClientBubbleHandler& Bubble = BubbleInfo.GetWarSerializer().Bubble;

		// Team/owner don't change after AddEntityCallback set them once - only position/yaw needs continuous updates.
		PositionYawHandler.ModifyEntity<FMassWarFastArrayItem>(Handle, EntityIdx, Bubble.GetTransformHandlerMutable());
	};

	auto RemoveEntityCallback = [&](FMassExecutionContext& Context, const FMassReplicatedAgentHandle Handle, const FMassClientHandle ClientHandle)
	{
		AMassWarClientBubbleInfo& BubbleInfo = RepSharedFrag->GetTypedClientBubbleInfoChecked<AMassWarClientBubbleInfo>(ClientHandle);
		BubbleInfo.GetWarSerializer().Bubble.RemoveAgentChecked(Handle);
	};

	CalculateClientReplication<FMassWarFastArrayItem>(Context, ReplicationContext, CacheViewsCallback, AddEntityCallback, ModifyEntityCallback, RemoveEntityCallback);
#endif // UE_REPLICATION_COMPILE_SERVER_CODE
}
