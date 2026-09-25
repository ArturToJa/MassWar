// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimInstance.h"
#include "MassWarPuppetAnimInstance.generated.h"

class AMassWarUnitVisualCharacter;

/**
 * Optional parent class for the animation blueprint of an AMassWarUnitVisualCharacter puppet. It copies
 * the puppet's VisualState into plain variables once per update, so the anim graph just reads variables
 * (state machine transitions, blend space inputs) - no per-frame casts, no Blueprint event-graph work per
 * puppet, and nothing read from the movement component (a puppet's movement component is not simulated).
 *
 * The plugin ships no animation assets; the game builds its own anim blueprint and sets this class as its
 * Parent Class (Class Settings).
 */
UCLASS()
class MASSWAREMBODIMENT_API UMassWarPuppetAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** Horizontal speed in cm/s - drive the locomotion blend space with this. */
	UPROPERTY(BlueprintReadOnly, Category = "MassWar|Animation")
	float UnitSpeed = 0.f;

	/** True while the unit is actually moving - use as the idle <-> locomotion transition condition. */
	UPROPERTY(BlueprintReadOnly, Category = "MassWar|Animation")
	bool bUnitIsMoving = false;

	/** True once the unit has died (it lingers as a corpse for its DeathLingerTime) - use as the transition
	 *  into a Death state, and its negation to leave it (a pooled puppet gets reused for a living unit). */
	UPROPERTY(BlueprintReadOnly, Category = "MassWar|Animation")
	bool bUnitIsDead = false;

	/** Which death animation to play, in [0, DeathVariantCount) - see AMassWarUnitVisualCharacter. */
	UPROPERTY(BlueprintReadOnly, Category = "MassWar|Animation")
	int32 UnitDeathVariant = 0;

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	TWeakObjectPtr<const AMassWarUnitVisualCharacter> Puppet;
};
