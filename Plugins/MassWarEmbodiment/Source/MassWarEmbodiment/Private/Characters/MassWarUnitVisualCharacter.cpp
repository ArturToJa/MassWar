// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/MassWarUnitVisualCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"

AMassWarUnitVisualCharacter::AMassWarUnitVisualCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	AIControllerClass = nullptr;
	AutoPossessAI = EAutoPossessAI::Disabled;

	// Every machine runs its own Mass representation for its own camera - see the class comment.
	bReplicates = false;
	SetReplicateMovement(false);

	// Purely visual: it must not block other units, the RTS camera, or the ground-click trace
	// (AMassWarSelectionPlayerController traces ECC_Visibility for move orders).
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// SyncFromEntity places the Actor, so the movement component has nothing to do. It is left untouched
	// (its motion values stay at their defaults) - animation reads VisualState, not the movement component.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->SetComponentTickEnabled(false);
		Movement->bOrientRotationToMovement = false;
	}

	// Crowd defaults: many of these can be on screen, and animation is the main per-puppet cost. Update
	// rate optimization lowers the evaluation rate of distant / small-on-screen meshes (interpolating in
	// between), and off-screen meshes skip pose work entirely. Tunable per Blueprint on the Mesh component.
	if (USkeletalMeshComponent* SkeletalMesh = GetMesh())
	{
		SkeletalMesh->bEnableUpdateRateOptimizations = true;
		SkeletalMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	}
}

void AMassWarUnitVisualCharacter::PlayAttack(const uint8 AttackCounter, const FMassEntityHandle Entity)
{
	if (AttackMontages.Num() > 0)
	{
		if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
		{
			// Round-robin on the counter, offset per unit so neighbours don't all swing the same clip at once.
			// Purely cosmetic and local to this machine - there is no random state to keep in sync.
			const uint32 Pick = static_cast<uint32>(AttackCounter) + GetTypeHash(Entity);
			if (UAnimMontage* Montage = AttackMontages[Pick % static_cast<uint32>(AttackMontages.Num())])
			{
				AnimInstance->Montage_Play(Montage);
			}
		}
	}

	OnUnitAttacked();
}

void AMassWarUnitVisualCharacter::StopAttackMontages()
{
	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		if (AnimInstance->IsAnyMontagePlaying())
		{
			AnimInstance->Montage_Stop(0.1f);
		}
	}
}

void AMassWarUnitVisualCharacter::SyncFromEntity(FMassEntityHandle Entity, const FTransform& EntityTransform, const float DeltaTime, const FMassWarPuppetEntityState& EntityState)
{
	const bool bIsDying = EntityState.bIsDying;

	const UWorld* World = GetWorld();
	const double Now = World ? World->GetTimeSeconds() : 0.0;
	const FVector NewLocation = EntityTransform.GetLocation();

	// Only trust the frame-to-frame delta if this puppet was last synced to this same entity roughly one
	// frame ago; otherwise (freshly spawned, pooled Actor reassigned, re-enabled after being an ISM
	// instance) the delta is a teleport, not motion.
	const bool bContinuous = Entity == LastSyncedEntity
		&& DeltaTime > UE_SMALL_NUMBER
		&& (Now - LastSyncTimeSeconds) <= FMath::Max(0.25, 4.0 * DeltaTime);

	if (bContinuous)
	{
		// Smoothed: a raw position delta between two frames is noisy (frame-time jitter, replication
		// interpolation steps) and would make a speed-driven blend space shake.
		const FVector RawVelocity = (NewLocation - LastSyncedLocation) / DeltaTime;
		VisualState.Velocity = FMath::VInterpTo(VisualState.Velocity, RawVelocity, DeltaTime, VelocitySmoothing);
		VisualState.Speed = VisualState.Velocity.Size2D();
	}
	else
	{
		VisualState.Velocity = FVector::ZeroVector;
		VisualState.Speed = 0.f;
	}
	VisualState.bIsMoving = VisualState.Speed > MovingSpeedThreshold;

	// Attack animation. Only a *change* of the counter seen while continuously showing the same unit counts as
	// an attack: a freshly assigned unit (or one that just came back from being an ISM instance) only sets the
	// baseline, and whatever the puppet was doing for its previous unit is cut off.
	if (bContinuous)
	{
		if (bIsDying)
		{
			if (!VisualState.bIsDead)
			{
				StopAttackMontages(); // the death animation takes over
			}
		}
		else if (EntityState.AttackCounter != LastAttackCounter)
		{
			PlayAttack(EntityState.AttackCounter, Entity);
		}
	}
	else
	{
		StopAttackMontages();
	}
	LastAttackCounter = EntityState.AttackCounter;

	// Written every sync, so a pooled puppet handed a living unit after a dead one drops the dead state.
	VisualState.bIsDead = bIsDying;
	VisualState.DeathVariant = (bIsDying && DeathVariantCount > 1) ? static_cast<int32>(GetTypeHash(Entity) % static_cast<uint32>(DeathVariantCount)) : 0;
	if (bIsDying)
	{
		// A dead unit is not moving, whatever smoothing residue the velocity still carries.
		VisualState.Velocity = FVector::ZeroVector;
		VisualState.Speed = 0.f;
		VisualState.bIsMoving = false;
	}

	LastSyncedEntity = Entity;
	LastSyncedLocation = NewLocation;
	LastSyncTimeSeconds = Now;

	SetActorLocationAndRotation(NewLocation, EntityTransform.GetRotation(), /*bSweep=*/ false, /*OutSweepHitResult=*/ nullptr, ETeleportType::TeleportPhysics);
}
