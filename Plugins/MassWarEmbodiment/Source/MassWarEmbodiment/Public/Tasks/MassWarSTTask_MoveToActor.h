// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "StateTreeTaskBase.h"
#include "MassWarSTTask_MoveToActor.generated.h"

class UMassWarUnitStateComponent;

USTRUCT()
struct FMassWarSTTask_MoveToActorInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Input)
	FVector Destination = FVector::ZeroVector;
};

/**
 * Actor-schema counterpart to MassWarStateTreeAI's FMassWarSTTask_MoveTo - writes a Move order via the
 * same FMassWarUnitStateView::RequestMoveTo, but reads/writes it through UMassWarUnitStateComponent
 * (auto-resolved by UStateTreeComponentSchema) instead of a Mass fragment. AMassWarUnitCharacter is
 * responsible for actually moving, driven by the order this task writes.
 */
USTRUCT(meta = (DisplayName = "MassWar Move To (Actor)"))
struct MASSWAREMBODIMENT_API FMassWarSTTask_MoveToActor : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FMassWarSTTask_MoveToActorInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;

	TStateTreeExternalDataHandle<UMassWarUnitStateComponent> StateHandle;
};
