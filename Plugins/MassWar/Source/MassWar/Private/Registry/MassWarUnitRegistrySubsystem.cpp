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

TArray<FMassWarUnitHandle> UMassWarUnitRegistrySubsystem::GetAllUnitHandles() const
{
	TArray<FMassWarUnitHandle> Handles;
	Handles.Reserve(Units.Num());

	for (const FMassEntityHandle& Entity : Units)
	{
		Handles.Add(FMassWarUnitHandle(Entity));
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
