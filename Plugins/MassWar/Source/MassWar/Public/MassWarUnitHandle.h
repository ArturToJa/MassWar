// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityHandle.h"
#include "MassWarUnitHandle.generated.h"

/**
 * Refers to one MassWar unit. Today every unit is a Mass entity, so this only wraps an FMassEntityHandle;
 * it stays a distinct USTRUCT (rather than a bare FMassEntityHandle) because StateTree nodes expose it as
 * instance data, and changing that property's type would invalidate already-compiled StateTree assets.
 */
USTRUCT()
struct MASSWAR_API FMassWarUnitHandle
{
	GENERATED_BODY()

	FMassWarUnitHandle() = default;
	explicit FMassWarUnitHandle(FMassEntityHandle InEntity) : Entity(InEntity) {}

	bool IsValid() const { return Entity.IsValid(); }

	bool operator==(const FMassWarUnitHandle& Other) const
	{
		return Entity == Other.Entity;
	}

	UPROPERTY()
	FMassEntityHandle Entity;
};
