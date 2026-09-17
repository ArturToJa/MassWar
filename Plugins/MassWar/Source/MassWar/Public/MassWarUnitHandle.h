// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityHandle.h"
#include "MassWarUnitHandle.generated.h"

/**
 * Refers to one MassWar unit regardless of how it's embodied: an ordinary Mass entity, or (once
 * MassWarEmbodiment is installed) a standalone Actor that opted out of Mass entirely. Exactly one of
 * Entity/Actor is set for a valid handle - AI/gameplay code that needs to treat both uniformly (see
 * FMassWarUnitStateView, UMassWarUnitRegistrySubsystem::GetAllUnitHandles) goes through this instead of
 * a bare FMassEntityHandle.
 */
USTRUCT()
struct MASSWAR_API FMassWarUnitHandle
{
	GENERATED_BODY()

	FMassWarUnitHandle() = default;
	explicit FMassWarUnitHandle(FMassEntityHandle InEntity) : Entity(InEntity) {}
	explicit FMassWarUnitHandle(AActor* InActor) : Actor(InActor) {}

	bool IsMassEntity() const { return Entity.IsValid(); }
	bool IsActor() const { return Actor.IsValid(); }
	bool IsValid() const { return IsMassEntity() || IsActor(); }

	bool operator==(const FMassWarUnitHandle& Other) const
	{
		return Entity == Other.Entity && Actor == Other.Actor;
	}

	UPROPERTY()
	FMassEntityHandle Entity;

	UPROPERTY()
	TWeakObjectPtr<AActor> Actor;
};
