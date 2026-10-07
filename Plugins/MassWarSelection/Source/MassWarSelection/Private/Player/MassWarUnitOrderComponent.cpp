// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/MassWarUnitOrderComponent.h"
#include "Player/MassWarSelectionPlayerController.h"
#include "MassSpawnerSubsystem.h"
#include "MassEntityManager.h"
#include "Fragments/MassWarUnitFragments.h"
#include "UnitBrain/MassWarUnitStateView.h"
#include "Registry/MassWarUnitRegistrySubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Engine/NetConnection.h"
#include "Formation/MassWarFormationSubsystem.h"
#include "MassEntityUtils.h"

UMassWarUnitOrderComponent::UMassWarUnitOrderComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

FMassEntityHandle UMassWarUnitOrderComponent::ResolveTarget(FMassEntityManager& EntityManager, const FMassWarOrderTarget& Target) const
{
	// A remote client's FMassEntityHandle only means something in that client's own local (replicated)
	// Mass world. Both worlds allocate handles from similarly-numbered sequential counters, so a remote
	// client's handle routinely *coincidentally* matches {Index, SerialNumber} of a completely different,
	// unrelated (but currently valid) entity in the server's own EntityManager - IsEntityValid() alone
	// can't tell the difference, it would silently apply the order to the wrong unit. Only the listen
	// server's own local player (no NetConnection - GetNetConnection() is null exactly for the local
	// player) is guaranteed to share the server's actual Mass world, where the handle is unambiguous.
	const APlayerController* OwningController = GetOwner<APlayerController>();
	const bool bIsLocalConnection = OwningController && OwningController->GetNetConnection() == nullptr;

	if (bIsLocalConnection && EntityManager.IsEntityValid(Target.Handle))
	{
		return Target.Handle;
	}

	if (UMassWarUnitRegistrySubsystem* Registry = GetWorld() ? GetWorld()->GetSubsystem<UMassWarUnitRegistrySubsystem>() : nullptr)
	{
		return Registry->FindByNetId(EntityManager, Target.NetId);
	}

	return FMassEntityHandle();
}

bool UMassWarUnitOrderComponent::IsOwnedByCallingPlayer(FMassEntityManager& EntityManager, FMassEntityHandle Entity) const
{
	const AMassWarSelectionPlayerController* OwningController = GetOwner<AMassWarSelectionPlayerController>();
	const FMassWarOwnerFragment* Owner = EntityManager.GetFragmentDataPtr<FMassWarOwnerFragment>(Entity);
	return OwningController && Owner && Owner->OwningPlayerId == OwningController->GetPlayerId();
}

void UMassWarUnitOrderComponent::ServerIssueMoveOrder_Implementation(const TArray<FMassWarOrderTarget>& Entities, FVector Destination)
{
	UMassSpawnerSubsystem* Spawner = GetWorld() ? GetWorld()->GetSubsystem<UMassSpawnerSubsystem>() : nullptr;
	if (!Spawner)
	{
		return;
	}

	FMassEntityManager& EntityManager = Spawner->GetEntityManagerChecked();
	for (const FMassWarOrderTarget& OrderTarget : Entities)
	{
		const FMassEntityHandle Entity = ResolveTarget(EntityManager, OrderTarget);
		if (!FMassWarUnitStateView::IsLiving(EntityManager, Entity) || !IsOwnedByCallingPlayer(EntityManager, Entity))
		{
			continue;
		}

		if (FMassWarOrderFragment* Order = EntityManager.GetFragmentDataPtr<FMassWarOrderFragment>(Entity))
		{
			Order->OrderType = EMassWarOrderType::Move;
			Order->Destination = Destination;
			Order->TargetEntity.Reset();
			Order->bStopAtAttackDistance = false; // a click order finishes where you clicked
			Order->bPlayerCommanded = true;
		}
	}
}

void UMassWarUnitOrderComponent::ServerIssueAttackOrder_Implementation(const TArray<FMassWarOrderTarget>& Entities, FMassWarOrderTarget Target)
{
	UMassSpawnerSubsystem* Spawner = GetWorld() ? GetWorld()->GetSubsystem<UMassSpawnerSubsystem>() : nullptr;
	if (!Spawner)
	{
		return;
	}

	FMassEntityManager& EntityManager = Spawner->GetEntityManagerChecked();
	const FMassEntityHandle TargetEntity = ResolveTarget(EntityManager, Target);
	if (!FMassWarUnitStateView::IsLiving(EntityManager, TargetEntity))
	{
		return;
	}

	for (const FMassWarOrderTarget& OrderTarget : Entities)
	{
		const FMassEntityHandle Entity = ResolveTarget(EntityManager, OrderTarget);
		if (!FMassWarUnitStateView::IsLiving(EntityManager, Entity) || Entity == TargetEntity || !IsOwnedByCallingPlayer(EntityManager, Entity))
		{
			continue;
		}

		if (FMassWarOrderFragment* Order = EntityManager.GetFragmentDataPtr<FMassWarOrderFragment>(Entity))
		{
			Order->OrderType = EMassWarOrderType::Attack;
			Order->TargetEntity = TargetEntity;
			Order->bPlayerCommanded = true;
		}
	}
}

void UMassWarUnitOrderComponent::ServerIssueFormationMoveOrder_Implementation(const TArray<int32>& FormationIds, FVector Destination)
{
	UWorld* World = GetWorld();
	UMassWarFormationSubsystem* Formations = World ? World->GetSubsystem<UMassWarFormationSubsystem>() : nullptr;
	const AMassWarSelectionPlayerController* OwningController = GetOwner<AMassWarSelectionPlayerController>();
	if (!Formations || !OwningController)
	{
		return;
	}

	TArray<uint32> Ids;
	Ids.Reserve(FormationIds.Num());
	for (const int32 Id : FormationIds)
	{
		Ids.Add(static_cast<uint32>(Id));
	}
	Formations->IssueMoveOrder(UE::Mass::Utils::GetEntityManagerChecked(*World), Ids, Destination, OwningController->GetPlayerId());
}

void UMassWarUnitOrderComponent::ServerIssueFormationAttackOrder_Implementation(const TArray<int32>& FormationIds, int32 TargetFormationId)
{
	UWorld* World = GetWorld();
	UMassWarFormationSubsystem* Formations = World ? World->GetSubsystem<UMassWarFormationSubsystem>() : nullptr;
	const AMassWarSelectionPlayerController* OwningController = GetOwner<AMassWarSelectionPlayerController>();
	if (!Formations || !OwningController)
	{
		return;
	}

	TArray<uint32> Ids;
	Ids.Reserve(FormationIds.Num());
	for (const int32 Id : FormationIds)
	{
		Ids.Add(static_cast<uint32>(Id));
	}
	Formations->IssueAttackOrder(UE::Mass::Utils::GetEntityManagerChecked(*World), Ids, static_cast<uint32>(TargetFormationId), OwningController->GetPlayerId());
}
