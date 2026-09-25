// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassStateTreeTypes.h"
#include "MassWarUnitHandle.h"
#include "Fragments/MassWarPerceptionFragments.h"
#include "MassWarSTEval_PerceivedEnemy.generated.h"

struct FTransformFragment;
struct FMassWarFormationMemberFragment;
class UMassWarFormationSubsystem;

USTRUCT()
struct FMassWarSTEval_PerceivedEnemyInstanceData
{
	GENERATED_BODY()

	/** Also report an enemy that is no longer in sight but was seen, heard or felt recently (its last known
	 *  location is reported). Off = only enemies in sight right now. */
	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bIncludeRemembered = true;

	/** Also use what the rest of the unit's formation perceives: an enemy any member sees counts as seen by this
	 *  unit too. Off = only what this unit perceives itself. */
	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bUseFormationKnowledge = true;

	/** Remembered enemies older than this (seconds) are ignored. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.0"))
	float MaxMemoryAge = 5.f;

	/** bTookDamageRecently is true if the unit was hurt within this many seconds. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = "0.0"))
	float RecentDamageWindow = 3.f;

	UPROPERTY(EditAnywhere, Category = Output)
	bool bHasEnemy = false;

	UPROPERTY(EditAnywhere, Category = Output)
	FMassWarUnitHandle NearestEnemy;

	/** Where the enemy is - or, if it is only remembered, where it was last perceived. */
	UPROPERTY(EditAnywhere, Category = Output)
	FVector NearestEnemyLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = Output)
	float DistanceToNearestEnemy = 0.f;

	/** True if the enemy is in sight right now; false if it is a memory. */
	UPROPERTY(EditAnywhere, Category = Output)
	bool bEnemyInSight = false;

	/** Which sense last told the unit about that enemy. */
	UPROPERTY(EditAnywhere, Category = Output)
	EMassWarSense LastSense = EMassWarSense::Sight;

	/** Seconds since the unit last perceived that enemy (0 while in sight). */
	UPROPERTY(EditAnywhere, Category = Output)
	float TimeSincePerceived = 0.f;

	UPROPERTY(EditAnywhere, Category = Output)
	bool bTookDamageRecently = false;
};

/**
 * Perception-based replacement for "MassWar Find Nearest Enemy": instead of knowing where every enemy on the
 * map is, the unit only reacts to enemies it has seen, heard or been hurt by (MassWarPerception), and can chase
 * the last known location of one that slipped out of sight. Same outputs as the omniscient evaluator (plus a
 * few perception details), so HasEnemyInRange / AttackTarget / MoveTo bind to it the same way.
 */
USTRUCT(meta = (DisplayName = "MassWar Perceived Enemy"))
struct MASSWARSTATETREEAI_API FMassWarSTEval_PerceivedEnemy : public FMassStateTreeEvaluatorBase
{
	GENERATED_BODY()

	using FInstanceDataType = FMassWarSTEval_PerceivedEnemyInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual void Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;

	TStateTreeExternalDataHandle<FTransformFragment> TransformHandle;
	TStateTreeExternalDataHandle<FMassWarPerceptionFragment> PerceptionHandle;
	TStateTreeExternalDataHandle<FMassWarFormationMemberFragment> FormationMemberHandle;
	TStateTreeExternalDataHandle<UMassWarFormationSubsystem> FormationSubsystemHandle;
};
