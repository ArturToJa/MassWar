// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Character.h"
#include "MassEntityHandle.h"
#include "Characters/MassWarVisualPuppetInterface.h"
#include "MassWarUnitVisualCharacter.generated.h"

class UAnimMontage;

/**
 * What animation (or any other cosmetic code) can read about the Mass entity this puppet is currently
 * standing in for. Derived every frame from the entity's transform only - not from Mass velocity/order
 * fragments - because a replicated client copy of a unit carries just position and yaw, so this is the one
 * signal that behaves identically on server, standalone and client. This is the single source of motion
 * data for a puppet: nothing is written into the movement component. Animation normally reads it through
 * UMassWarPuppetAnimInstance rather than casting to the puppet itself.
 */
USTRUCT(BlueprintType)
struct MASSWAREMBODIMENT_API FMassWarVisualState
{
	GENERATED_BODY()

	/** World-space velocity, from the entity's frame-to-frame movement. */
	UPROPERTY(BlueprintReadOnly, Category = "MassWar|Visual")
	FVector Velocity = FVector::ZeroVector;

	/** Horizontal speed in cm/s. */
	UPROPERTY(BlueprintReadOnly, Category = "MassWar|Visual")
	float Speed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "MassWar|Visual")
	bool bIsMoving = false;

	/** The unit is dead and lingering (its entity is Dying) - play the death animation. It stays true until
	 *  the puppet is released or handed to another unit; a dead unit is neither moving nor attacking. */
	UPROPERTY(BlueprintReadOnly, Category = "MassWar|Visual")
	bool bIsDead = false;

	/** Stable per-unit index in [0, DeathVariantCount) for picking one of several death animations. Always 0
	 *  while alive. */
	UPROPERTY(BlueprintReadOnly, Category = "MassWar|Visual")
	int32 DeathVariant = 0;
};

/**
 * Cosmetic-only near-LOD puppet for ordinary Mass units - configured as an ordinary Mass visualization
 * trait's HighResTemplateActor, so Mass's own
 * MassRepresentationActorManagement spawns/despawns it when the camera gets close.
 *
 * The Mass entity is, and always stays, the unit: its health, order, StateTree and replication never move
 * into this Actor, and the Actor going away (camera pulls back, unit dies) loses nothing. This class only
 * *displays* the entity - it has no AI controller, no gameplay data and no simulation authority. Each
 * frame UMassWarVisualSyncProcessor calls SyncFromEntity() to place/orient it and refresh VisualState.
 *
 * Deliberately NOT replicated: every machine runs its own Mass representation for its own camera, so a
 * server-spawned puppet replicating to clients would show up as a duplicate ghost unit (and would leak
 * enemy positions that fog of war withholds).
 *
 * Abstract and content-free on purpose: the plugin ships no meshes, skeletons or animation. The game
 * derives its own class (C++ or Blueprint), supplies the visuals (static/skeletal mesh, anim blueprint
 * reading VisualState or CharacterMovement velocity), and points the unit config's HighResTemplateActor at
 * it. Games that don't want a Character can skip this class and implement IMassWarVisualPuppet directly.
 */
UCLASS(Abstract)
class MASSWAREMBODIMENT_API AMassWarUnitVisualCharacter : public ACharacter, public IMassWarVisualPuppet
{
	GENERATED_BODY()

public:
	AMassWarUnitVisualCharacter();

	//~ IMassWarVisualPuppet
	virtual void SyncFromEntity(FMassEntityHandle Entity, const FTransform& EntityTransform, float DeltaTime, const FMassWarPuppetEntityState& EntityState) override;

	const FMassWarVisualState& GetVisualState() const { return VisualState; }

	/**
	 * Montages played, one per attack the unit lands, through whatever slot each one targets (the anim
	 * blueprint needs a matching Slot node, normally DefaultSlot). They are picked round-robin from the unit's
	 * attack counter, offset per unit so neighbours vary. Leave empty for no attack animation. The plugin
	 * ships none - assign your own on the Blueprint.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MassWar|Animation")
	TArray<TObjectPtr<UAnimMontage>> AttackMontages;

protected:
	/** Called every time this puppet's unit lands an attack (after the montage, if any, has started) - for
	 *  attack effects such as sounds or particles. Not called for attacks that happened while the puppet
	 *  wasn't showing the unit. */
	UFUNCTION(BlueprintImplementableEvent, Category = "MassWar|Animation")
	void OnUnitAttacked();

	UPROPERTY(BlueprintReadOnly, Category = "MassWar|Visual")
	FMassWarVisualState VisualState;

	/** Speed (cm/s) above which VisualState.bIsMoving is set. */
	UPROPERTY(EditAnywhere, Category = "MassWar|Visual")
	float MovingSpeedThreshold = 10.f;

	/** How many death animations the unit's anim blueprint can choose between; VisualState.DeathVariant is
	 *  spread evenly over [0, this). Leave at 1 if there is a single death animation. */
	UPROPERTY(EditAnywhere, Category = "MassWar|Visual", meta = (ClampMin = "1"))
	int32 DeathVariantCount = 1;

	/** How quickly the published velocity follows the entity's raw movement (higher = snappier, noisier). */
	UPROPERTY(EditAnywhere, Category = "MassWar|Visual", meta = (ClampMin = "0.0"))
	float VelocitySmoothing = 12.f;

private:
	/** Mass pools and re-uses released puppets, so the Actor can be handed to a different entity (or the
	 *  same one after a gap) - a frame-to-frame velocity is only meaningful against the same entity's
	 *  immediately preceding sync. */
	FMassEntityHandle LastSyncedEntity;
	FVector LastSyncedLocation = FVector::ZeroVector;
	double LastSyncTimeSeconds = 0.0;

	/** Attack counter last seen for the unit being shown - see FMassWarAttackFeedbackFragment. */
	uint8 LastAttackCounter = 0;

	void PlayAttack(uint8 AttackCounter, FMassEntityHandle Entity);
	void StopAttackMontages();
};
