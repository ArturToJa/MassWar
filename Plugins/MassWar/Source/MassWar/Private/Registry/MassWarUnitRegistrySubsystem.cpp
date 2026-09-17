// Copyright Epic Games, Inc. All Rights Reserved.

#include "Registry/MassWarUnitRegistrySubsystem.h"
#include "MassEntityManager.h"
#include "Fragments/MassWarUnitFragments.h"

void UMassWarUnitRegistrySubsystem::RegisterUnit(FMassEntityHandle Entity)
{
	Units.AddUnique(Entity);
}

void UMassWarUnitRegistrySubsystem::UnregisterUnit(FMassEntityHandle Entity)
{
	Units.RemoveSingleSwap(Entity);
}

void UMassWarUnitRegistrySubsystem::RegisterActorUnit(AActor* Actor)
{
	if (Actor)
	{
		ActorUnits.AddUnique(Actor);
	}
}

void UMassWarUnitRegistrySubsystem::UnregisterActorUnit(AActor* Actor)
{
	ActorUnits.RemoveSingleSwap(Actor);
}

TArray<FMassWarUnitHandle> UMassWarUnitRegistrySubsystem::GetAllUnitHandles() const
{
	TArray<FMassWarUnitHandle> Handles;
	Handles.Reserve(Units.Num() + ActorUnits.Num());

	for (const FMassEntityHandle& Entity : Units)
	{
		Handles.Add(FMassWarUnitHandle(Entity));
	}

	for (const TWeakObjectPtr<AActor>& Actor : ActorUnits)
	{
		if (AActor* ActorPtr = Actor.Get())
		{
			Handles.Add(FMassWarUnitHandle(ActorPtr));
		}
	}

	return Handles;
}

FMassEntityHandle UMassWarUnitRegistrySubsystem::FindByNetId(const FMassEntityManager& EntityManager, uint32 NetId) const
{
	if (NetId == 0)
	{
		return FMassEntityHandle();
	}

	for (const FMassEntityHandle& Entity : Units)
	{
		if (!EntityManager.IsEntityValid(Entity))
		{
			continue;
		}

		const FMassWarNetIdFragment* NetIdFragment = EntityManager.GetFragmentDataPtr<FMassWarNetIdFragment>(Entity);
		if (NetIdFragment && NetIdFragment->NetId == NetId)
		{
			return Entity;
		}
	}

	return FMassEntityHandle();
}
