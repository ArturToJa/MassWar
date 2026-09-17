// Copyright Epic Games, Inc. All Rights Reserved.

#include "Replication/MassWarClientBubble.h"
#include "Fragments/MassWarUnitFragments.h"
#include "Registry/MassWarUnitRegistrySubsystem.h"
#include "Smoothing/MassWarClientInterpolationFragment.h"
#include "Net/UnrealNetwork.h"
#include "MassExecutionContext.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassWarClientBubble)

#if UE_REPLICATION_COMPILE_CLIENT_CODE
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
