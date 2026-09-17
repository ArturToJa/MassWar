// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassStateTreeTypes.h"
#include "MassWarUnitHandle.h"
#include "MassWarSTEval_FindNearestEnemy.generated.h"

class UMassWarUnitRegistrySubsystem;
struct FTransformFragment;
struct FMassWarTeamFragment;

USTRUCT()
struct FMassWarSTEval_FindNearestEnemyInstanceData
{
	GENERATED_BODY()

	/** How far to look for an enemy. Deliberately map-wide by default - this scaffold has no
	 *  vision/fog-of-war yet (that's MassWarFogOfWar, a later pass), so "can find" == "can see". */
	UPROPERTY(EditAnywhere, Category = Parameter)
	float SearchRadius = 50000.f;

	UPROPERTY(EditAnywhere, Category = Output)
	bool bHasEnemy = false;

	UPROPERTY(EditAnywhere, Category = Output)
	FMassWarUnitHandle NearestEnemy;

	UPROPERTY(EditAnywhere, Category = Output)
	FVector NearestEnemyLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = Output)
	float DistanceToNearestEnemy = 0.f;
};

/**
 * Continuously tracks the nearest enemy unit (if any) within SearchRadius, for other nodes to bind to
 * (HasEnemyInRange condition, AttackTarget task's Target input, MoveTo's Destination input). Candidates
 * come from UMassWarUnitRegistrySubsystem::GetAllUnitHandles(), so this finds both ordinary Mass units
 * and (MassWarEmbodiment) embodied Actor units alike - the actual search is
 * FMassWarUnitStateView::FindNearestEnemy(), shared with FMassWarSTEval_FindNearestEnemyActor so both
 * StateTree schemas run identical decision logic.
 */
USTRUCT(meta = (DisplayName = "MassWar Find Nearest Enemy"))
struct MASSWARSTATETREEAI_API FMassWarSTEval_FindNearestEnemy : public FMassStateTreeEvaluatorBase
{
	GENERATED_BODY()

	using FInstanceDataType = FMassWarSTEval_FindNearestEnemyInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual void Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;

	TStateTreeExternalDataHandle<FTransformFragment> TransformHandle;
	TStateTreeExternalDataHandle<FMassWarTeamFragment> TeamHandle;
	TStateTreeExternalDataHandle<UMassWarUnitRegistrySubsystem> RegistryHandle;
};
