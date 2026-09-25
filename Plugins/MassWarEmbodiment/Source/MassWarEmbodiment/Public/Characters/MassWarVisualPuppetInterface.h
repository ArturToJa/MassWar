// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityHandle.h"
#include "UObject/Interface.h"
#include "MassWarVisualPuppetInterface.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UMassWarVisualPuppet : public UInterface
{
	GENERATED_BODY()
};

/** The parts of a Mass entity's state a puppet may want to show, handed to it every frame. */
struct FMassWarPuppetEntityState
{
	/** The entity is dead and lingering (FMassWarLifeFragment) - show its death. */
	bool bIsDying = false;

	/** FMassWarAttackFeedbackFragment::AttackCounter - an event counter that changes each time the unit lands
	 *  an attack; compare it with the last value seen to know when to play an attack animation. */
	uint8 AttackCounter = 0;
};

/**
 * Contract between UMassWarVisualSyncProcessor and whatever Actor class the game uses as a Mass unit's
 * near-LOD visual (the unit config's HighResTemplateActor). The plugin ships no concrete visual - the game
 * defines its own Actor class and either derives from AMassWarUnitVisualCharacter (which implements this)
 * or implements this interface itself.
 */
class MASSWAREMBODIMENT_API IMassWarVisualPuppet
{
	GENERATED_BODY()

public:
	/** Called once per frame while this Actor is Entity's active representation: place/orient it at
	 *  EntityTransform and refresh any cosmetic state from EntityState. Data only ever flows entity -> actor.
	 *  Game thread only. */
	virtual void SyncFromEntity(FMassEntityHandle Entity, const FTransform& EntityTransform, float DeltaTime, const FMassWarPuppetEntityState& EntityState) = 0;
};
