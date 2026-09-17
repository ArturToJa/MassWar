// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/MassWarUnitOrderComponent.h"
#include "Player/MassWarSelectionPlayerController.h"
#include "MassSpawnerSubsystem.h"
#include "MassEntityManager.h"
#include "Fragments/MassWarUnitFragments.h"
#include "Registry/MassWarUnitRegistrySubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Engine/NetConnection.h"

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
		if (!EntityManager.IsEntityValid(Entity) || !IsOwnedByCallingPlayer(EntityManager, Entity))
		{
			continue;
		}

		if (FMassWarOrderFragment* Order = EntityManager.GetFragmentDataPtr<FMassWarOrderFragment>(Entity))
		{
			Order->OrderType = EMassWarOrderType::Move;
			Order->Destination = Destination;
			Order->TargetEntity.Reset();
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
	if (!EntityManager.IsEntityValid(TargetEntity))
	{
		return;
	}

	for (const FMassWarOrderTarget& OrderTarget : Entities)
	{
		const FMassEntityHandle Entity = ResolveTarget(EntityManager, OrderTarget);
		if (!EntityManager.IsEntityValid(Entity) || Entity == TargetEntity || !IsOwnedByCallingPlayer(EntityManager, Entity))
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
