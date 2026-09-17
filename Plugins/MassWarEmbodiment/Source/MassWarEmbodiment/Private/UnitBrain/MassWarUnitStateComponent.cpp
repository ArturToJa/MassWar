// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnitBrain/MassWarUnitStateComponent.h"
#include "Registry/MassWarUnitRegistrySubsystem.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassWarUnitStateComponent)

void UMassWarUnitStateComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UMassWarUnitRegistrySubsystem* Registry = World->GetSubsystem<UMassWarUnitRegistrySubsystem>())
		{
			Registry->RegisterActorUnit(GetOwner());
		}
	}
}

void UMassWarUnitStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UMassWarUnitRegistrySubsystem* Registry = World->GetSubsystem<UMassWarUnitRegistrySubsystem>())
		{
			Registry->UnregisterActorUnit(GetOwner());
		}
	}

	Super::EndPlay(EndPlayReason);
}
