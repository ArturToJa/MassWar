// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassStateTreeTypes.h"
#include "MassWarSTTask_MoveTo.generated.h"

struct FMassWarOrderFragment;

USTRUCT()
struct FMassWarSTTask_MoveToInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Input)
	FVector Destination = FVector::ZeroVector;

	/** True: the move is done once the unit is within its AttackStopDistance of Destination - right for an
	 *  approach-to-engage move toward an enemy, where getting into fighting range is enough. False: the
	 *  unit must actually arrive (within its small Move AcceptanceRadius). Defaults to true so trees built
	 *  before this option existed keep their units at fighting distance instead of running onto the enemy. */
	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bStopAtAttackDistance = true;
};

/**
 * Writes a Move order toward Destination and stays Running until MassWarOrderMovementProcessor (Core)
 * reports arrival by reverting the order back to Idle. No pathfinding - straight line, matching the
 * Core movement processor it relies on.
 */
USTRUCT(meta = (DisplayName = "MassWar Move To"))
struct MASSWARSTATETREEAI_API FMassWarSTTask_MoveTo : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FMassWarSTTask_MoveToInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual void GetDependencies(UE::MassBehavior::FStateTreeDependencyBuilder& Builder) const override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;

	TStateTreeExternalDataHandle<FMassWarOrderFragment> OrderHandle;
};
