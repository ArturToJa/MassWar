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
