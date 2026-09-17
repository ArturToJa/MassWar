// Copyright Epic Games, Inc. All Rights Reserved.

#include "Replication/MassWarReplicationEntityDestructor.h"
#include "Replication/MassWarClientBubble.h"
#include "Fragments/MassWarUnitFragments.h"
#include "MassReplicationSubsystem.h"
#include "MassExecutionContext.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassWarReplicationEntityDestructor)

UMassWarReplicationEntityDestructor::UMassWarReplicationEntityDestructor()
	: EntityQuery(*this)
{
	ObservedType = FMassWarNetIdFragment::StaticStruct();
	ObservedOperations = EMassObservedOperationFlags::Remove;
	ExecutionFlags = (int32)EProcessorExecutionFlags::Server;
	// Touches the bubble actors (UObjects) via GetTypedClientBubble.
	bRequiresGameThreadExecution = true;
}

void UMassWarReplicationEntityDestructor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassWarNetIdFragment>(EMassFragmentAccess::ReadOnly);
}

void UMassWarReplicationEntityDestructor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
#if UE_REPLICATION_COMPILE_SERVER_CODE
	UWorld* World = EntityManager.GetWorld();
	UMassReplicationSubsystem* ReplicationSubsystem = World ? UWorld::GetSubsystem<UMassReplicationSubsystem>(World) : nullptr;
	if (!ReplicationSubsystem)
	{
		return;
	}

	const FMassBubbleInfoClassHandle BubbleClassHandle = ReplicationSubsystem->GetBubbleInfoClassHandle(AMassWarClientBubbleInfo::StaticClass());
	if (!BubbleClassHandle.IsValid())
	{
		// MassWarReplication's bubble was never registered (e.g. no client ever connected this session) - nothing to clean up.
		return;
	}

	EntityQuery.ForEachEntityChunk(Context, [ReplicationSubsystem, BubbleClassHandle](FMassExecutionContext& Context)
	{
		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			const FMassEntityHandle Entity = Context.GetEntity(It);

			for (const FMassClientHandle ClientHandle : ReplicationSubsystem->GetClientReplicationHandles())
			{
				if (!ReplicationSubsystem->IsValidClientHandle(ClientHandle))
				{
					continue;
				}

				FMassClientReplicationInfo* ClientInfo = ReplicationSubsystem->GetMutableClientReplicationInfo(ClientHandle);
				if (!ClientInfo)
				{
					continue;
				}

				FMassReplicatedAgentData AgentData;
				if (ClientInfo->AgentsData.RemoveAndCopyValue(Entity, AgentData) && AgentData.Handle.IsValid())
				{
					if (AMassWarClientBubbleInfo* BubbleInfo = ReplicationSubsystem->GetTypedClientBubble<AMassWarClientBubbleInfo>(BubbleClassHandle, ClientHandle))
					{
						BubbleInfo->GetWarSerializer().Bubble.RemoveAgentChecked(AgentData.Handle);
					}
				}
			}
		}
	});
#endif // UE_REPLICATION_COMPILE_SERVER_CODE
}
