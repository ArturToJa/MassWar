// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"
#include "MassWarUnitStateProviderInterface.generated.h"

struct FMassWarOrderFragment;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UMassWarUnitStateProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by the Actor registered as a unit with UMassWarUnitRegistrySubsystem::RegisterActorUnit
 * (e.g. MassWarEmbodiment's AMassWarUnitAIController, forwarding to its UMassWarUnitStateComponent) so
 * FMassWarUnitStateView can back itself with either a Mass entity's fragments or a plain Actor, without
 * Core depending on MassWarEmbodiment - the dependency points the other way, same as
 * IMassWarTeamProvider does for MassWarFogOfWar.
 */
class MASSWAR_API IMassWarUnitStateProvider
{
	GENERATED_BODY()

public:
	virtual uint8 GetMassWarTeamId() const = 0;
	virtual FVector GetMassWarLocation() const = 0;
	virtual const FMassWarOrderFragment* GetMassWarOrder() const = 0;
	virtual FMassWarOrderFragment* GetMassWarOrderMutable() = 0;
};
