// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Character.h"
#include "MassWarUnitCharacter.generated.h"

class UStaticMeshComponent;

/**
 * A "hero" unit: a real, persistent ACharacter that opts out of Mass entirely but is driven by the same
 * StateTree decision logic as ordinary Mass units (via AMassWarUnitAIController running
 * ST_MassWarUnit_Actor, both schemas routing through the shared FMassWarUnitStateView). Movement and
 * damage-dealing are driven here from the order the StateTree writes, mirroring Core's
 * MassWarOrderMovementProcessor (straight-line chase, no pathfinding) and MassWarCombat's
 * MassWarDamageProcessor (range/interval-gated damage) respectively, since neither of those Mass
 * processors ever iterates this Actor.
 *
 * Visual: the stock UE5 mannequin skeleton/animations turned out not to be available as a plain
 * importable asset in this engine install without enabling Experimental Mover plugins (its own separate
 * risk), so per the Pass 7 plan's documented fallback this uses a plain primitive body mesh instead of a
 * skinned mannequin - still a real, persistent, StateTree-driven Character, just without idle/walk
 * animation. Swap in real art later without touching this class's logic.
 *
 * Scope cut (Pass 7): combat is one-directional - this hero can find and damage nearby Mass enemies via
 * UMassWarUnitRegistrySubsystem::OnDealDamage, but ordinary Mass units can't target/damage it back yet
 * (FMassWarUnitStateView::RequestAttack only ever writes an order when the target is a Mass entity).
 */
UCLASS()
class MASSWAREMBODIMENT_API AMassWarUnitCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMassWarUnitCharacter();

protected:
	UPROPERTY(VisibleAnywhere, Category = "MassWar|Visual")
	TObjectPtr<UStaticMeshComponent> BodyMesh;


	virtual void Tick(float DeltaSeconds) override;

	/** Straight-line movement toward the current order's destination (Move) or live target location
	 *  (Attack) - mirrors MassWarOrderMovementProcessor's chase behavior for parity with Mass units. */
	void TickMovement(float DeltaSeconds);

	/** Range/interval-gated damage against the current order's Attack target, dealt via
	 *  UMassWarUnitRegistrySubsystem::OnDealDamage - mirrors MassWarDamageProcessor without depending on
	 *  MassWarCombat's fragments. */
	void TickAttack(float DeltaSeconds);

	UPROPERTY(EditAnywhere, Category = "MassWar|Movement")
	float MoveSpeed = 500.f;

	UPROPERTY(EditAnywhere, Category = "MassWar|Movement")
	float AcceptanceRadius = 50.f;

	UPROPERTY(EditAnywhere, Category = "MassWar|Combat")
	float AttackDamage = 10.f;

	UPROPERTY(EditAnywhere, Category = "MassWar|Combat")
	float AttackRange = 800.f;

	UPROPERTY(EditAnywhere, Category = "MassWar|Combat")
	float AttackInterval = 1.f;

	UPROPERTY(Transient)
	float TimeSinceLastAttack = 0.f;
};
