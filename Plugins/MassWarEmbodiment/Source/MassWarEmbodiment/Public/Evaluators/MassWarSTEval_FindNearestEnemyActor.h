// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "StateTreeEvaluatorBase.h"
#include "StateTreeExecutionTypes.h"
#include "MassWarUnitHandle.h"
#include "MassWarSTEval_FindNearestEnemyActor.generated.h"

class UMassWarUnitStateComponent;
class UMassWarUnitRegistrySubsystem;
class UMassSpawnerSubsystem;

USTRUCT()
struct FMassWarSTEval_FindNearestEnemyActorInstanceData
{
	GENERATED_BODY()

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
 * Actor-schema counterpart to MassWarStateTreeAI's FMassWarSTEval_FindNearestEnemy - continuously tracks
 * the nearest enemy unit within SearchRadius for this Actor-embodied unit. Candidates come from
 * UMassWarUnitRegistrySubsystem::GetAllUnitHandles() (both ordinary Mass units and embodied Actors), and
 * the actual search is the shared FMassWarUnitStateView::FindNearestEnemy(), same as the Mass-schema
 * evaluator - the only difference is where "self" is read from (UMassWarUnitStateComponent instead of
 * Mass fragments) and where the FMassEntityManager needed to resolve Mass-entity candidates comes from
 * (UMassSpawnerSubsystem, since there's no Mass processor context here to supply one).
 */
USTRUCT(meta = (DisplayName = "MassWar Find Nearest Enemy (Actor)"))
struct MASSWAREMBODIMENT_API FMassWarSTEval_FindNearestEnemyActor : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FMassWarSTEval_FindNearestEnemyActorInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual void Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;

	TStateTreeExternalDataHandle<UMassWarUnitStateComponent> StateHandle;
	TStateTreeExternalDataHandle<UMassWarUnitRegistrySubsystem> RegistryHandle;
	TStateTreeExternalDataHandle<UMassSpawnerSubsystem> SpawnerHandle;
};
