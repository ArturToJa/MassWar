// Copyright Epic Games, Inc. All Rights Reserved.

#include "Replication/MassWarClientBubble.h"
#include "Fragments/MassWarUnitFragments.h"
#include "Registry/MassWarUnitRegistrySubsystem.h"
#include "Smoothing/MassWarClientInterpolationFragment.h"
#include "Net/UnrealNetwork.h"
#include "MassExecutionContext.h"
#include "Engine/World.h"
#include "Subsystem/MassWarReplicationSetupSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassWarClientBubble)

#if UE_REPLICATION_COMPILE_SERVER_CODE
void FMassWarClientBubbleHandler::SetAgentDynamicState(const FMassReplicatedAgentHandle Handle, const uint8 LifeState, const uint8 AttackCounter, const uint32 FormationId)
{
	check(AgentHandleManager.IsValidHandle(Handle));

	const int32 AgentsIdx = AgentLookupArray[Handle.GetIndex()].AgentsIdx;
	FMassWarFastArrayItem& Item = (*Agents)[AgentsIdx];

	if (Item.Agent.LifeState != LifeState || Item.Agent.AttackCounter != AttackCounter || Item.Agent.FormationId != FormationId)
	{
		Item.Agent.LifeState = LifeState;
		Item.Agent.AttackCounter = AttackCounter;
		Item.Agent.FormationId = FormationId;
		Serializer->MarkItemDirty(Item);
	}
}
#endif // UE_REPLICATION_COMPILE_SERVER_CODE

#if UE_REPLICATION_COMPILE_CLIENT_CODE
void FMassWarClientBubbleHandler::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	// Tell interested plugins (fog of war ghosts) where each unit was before the base class destroys its entity.
	UWorld* World = Serializer ? Serializer->GetWorld() : nullptr;
	if (UMassWarReplicationSetupSubsystem* Setup = World ? World->GetSubsystem<UMassWarReplicationSetupSubsystem>() : nullptr)
	{
		if (Setup->OnClientAgentRemoved.IsBound())
		{
			for (const int32 Index : RemovedIndices)
			{
				const FReplicatedWarAgent& Agent = (*Agents)[Index].Agent;
				Setup->OnClientAgentRemoved.Broadcast(Agent.GetNetID().GetValue(), Agent.GetReplicatedPositionYawData().GetPosition(),
					FMath::RadiansToDegrees(Agent.GetReplicatedPositionYawData().GetYaw()), Agent.TeamId, Agent.LifeState);
			}
		}
	}

	Super::PreReplicatedRemove(RemovedIndices, FinalSize);
}

void FMassWarClientBubbleHandler::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	auto AddRequirementsForSpawnQuery = [this](FMassEntityQuery& InQuery)
	{
		TransformHandler.AddRequirementsForSpawnQuery(InQuery);
	};

	auto CacheFragmentViewsForSpawnQuery = [this](FMassExecutionContext& InExecContext)
	{
		TransformHandler.CacheFragmentViewsForSpawnQuery(InExecContext);
	};

	auto SetSpawnedEntityData = [this](const FMassEntityView& EntityView, const FReplicatedWarAgent& ReplicatedEntity, const int32 EntityIdx)
	{
		TransformHandler.SetSpawnedEntityData(EntityIdx, ReplicatedEntity.GetReplicatedPositionYawData());

		if (FMassWarTeamFragment* Team = EntityView.GetFragmentDataPtr<FMassWarTeamFragment>())
		{
			Team->TeamId = ReplicatedEntity.TeamId;
		}

		if (FMassWarNetIdFragment* NetIdFragment = EntityView.GetFragmentDataPtr<FMassWarNetIdFragment>())
		{
			NetIdFragment->NetId = ReplicatedEntity.GetNetID().GetValue();
		}

		if (FMassWarOwnerFragment* Owner = EntityView.GetFragmentDataPtr<FMassWarOwnerFragment>())
		{
			Owner->OwningPlayerId = ReplicatedEntity.OwningPlayerId;
		}

		if (FMassWarLifeFragment* Life = EntityView.GetFragmentDataPtr<FMassWarLifeFragment>())
		{
			Life->State = static_cast<EMassWarLifeState>(ReplicatedEntity.LifeState);
		}

		if (FMassWarAttackFeedbackFragment* AttackFeedback = EntityView.GetFragmentDataPtr<FMassWarAttackFeedbackFragment>())
		{
			AttackFeedback->AttackCounter = ReplicatedEntity.AttackCounter;
		}

		if (FMassWarFormationMemberFragment* Formation = EntityView.GetFragmentDataPtr<FMassWarFormationMemberFragment>())
		{
			Formation->FormationId = ReplicatedEntity.FormationId;
		}

		// Initialize the smoothing target to the just-placed position/rotation so the first update we
		// receive later interpolates from here, not from the fragment's zeroed default.
		if (FMassWarClientInterpolationFragment* Interp = EntityView.GetFragmentDataPtr<FMassWarClientInterpolationFragment>())
		{
			const FTransform& Transform = EntityView.GetFragmentData<FTransformFragment>().GetTransform();
			Interp->TargetPosition = Transform.GetLocation();
			Interp->TargetRotation = Transform.GetRotation();
		}

		// Core's registry only ever gets populated by the server's own spawn code (AMassWarDemoGameMode),
		// which never runs on a client - without this, a client can see units but never select them.
		if (UWorld* World = Serializer->GetWorld())
		{
			if (UMassWarUnitRegistrySubsystem* Registry = World->GetSubsystem<UMassWarUnitRegistrySubsystem>())
			{
				Registry->RegisterUnit(EntityView.GetEntity());
			}

			// A unit that comes (back) into view: lets fog-of-war ghosts drop the one they kept for it.
			if (UMassWarReplicationSetupSubsystem* Setup = World->GetSubsystem<UMassWarReplicationSetupSubsystem>())
			{
				Setup->OnClientAgentAdded.Broadcast(ReplicatedEntity.GetNetID().GetValue());
			}
		}
	};

	auto SetModifiedEntityData = [this](const FMassEntityView& EntityView, const FReplicatedWarAgent& Item)
	{
		PostReplicatedChangeEntity(EntityView, Item);
	};

	PostReplicatedAddHelper(AddedIndices, AddRequirementsForSpawnQuery, CacheFragmentViewsForSpawnQuery, SetSpawnedEntityData, SetModifiedEntityData);

	TransformHandler.ClearFragmentViewsForSpawnQuery();
}

void FMassWarClientBubbleHandler::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	auto SetModifiedEntityData = [this](const FMassEntityView& EntityView, const FReplicatedWarAgent& Item)
	{
		PostReplicatedChangeEntity(EntityView, Item);
	};

	PostReplicatedChangeHelper(ChangedIndices, SetModifiedEntityData);
}

void FMassWarClientBubbleHandler::PostReplicatedChangeEntity(const FMassEntityView& EntityView, const FReplicatedWarAgent& Item) const
{
	// Team id is only ever set once, on add - MassWar has no re-teaming gameplay today.

	// Life state does change (Alive -> Dying): mirror it so this client's visual can animate the death.
	if (FMassWarLifeFragment* Life = EntityView.GetFragmentDataPtr<FMassWarLifeFragment>())
	{
		Life->State = static_cast<EMassWarLifeState>(Item.LifeState);
	}

	if (FMassWarAttackFeedbackFragment* AttackFeedback = EntityView.GetFragmentDataPtr<FMassWarAttackFeedbackFragment>())
	{
		AttackFeedback->AttackCounter = Item.AttackCounter;
	}

	if (FMassWarFormationMemberFragment* Formation = EntityView.GetFragmentDataPtr<FMassWarFormationMemberFragment>())
	{
		Formation->FormationId = Item.FormationId;
	}

	// Feed the smoothing target rather than hard-snapping FTransformFragment, so
	// UMassWarClientInterpolationProcessor can chase it smoothly instead of teleporting on every update.
	// Falls back to a hard snap if UMassWarReplicationSmoothingTrait isn't on this config.
	if (FMassWarClientInterpolationFragment* Interp = EntityView.GetFragmentDataPtr<FMassWarClientInterpolationFragment>())
	{
		Interp->TargetPosition = Item.GetReplicatedPositionYawData().GetPosition();
		Interp->TargetRotation = FQuat(FVector::UpVector, Item.GetReplicatedPositionYawData().GetYaw());
	}
	else
	{
		TransformHandler.SetModifiedEntityData(EntityView, Item.GetReplicatedPositionYawData());
	}
}
#endif // UE_REPLICATION_COMPILE_CLIENT_CODE

AMassWarClientBubbleInfo::AMassWarClientBubbleInfo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Serializers.Add(&WarSerializer);
}

void AMassWarClientBubbleInfo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	// Technically doesn't need to be push-model based since it's a FastArray and those ignore it, but matches engine convention.
	DOREPLIFETIME_WITH_PARAMS_FAST(AMassWarClientBubbleInfo, WarSerializer, SharedParams);
}
