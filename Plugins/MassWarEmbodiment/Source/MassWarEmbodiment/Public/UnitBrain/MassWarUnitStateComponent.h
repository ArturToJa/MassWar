// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "Fragments/MassWarUnitFragments.h"
#include "MassWarUnitStateComponent.generated.h"

/**
 * Actor-side data holder for the unit data Core's FMassWarUnitStateView needs (Team/Owner/Order) -
 * reuses Core's own fragment structs as plain members, since they're just data, not Mass-only. Attached
 * to AMassWarUnitAIController (not the Pawn): UStateTreeComponentSchema::CollectExternalData resolves a
 * TStateTreeExternalDataHandle<UActorComponent-derived> via FindComponentByClass on the StateTree
 * component's own Owner, which for a UStateTreeAIComponent is the AIController - so this component must
 * live there too for the Actor-schema StateTree nodes (MassWarSTTask_MoveToActor etc.) to find it with
 * no registration code.
 *
 * Deliberately does NOT implement IMassWarUnitStateProvider itself - FMassWarUnitHandle wraps an AActor,
 * and FMassWarUnitStateView::FromHandle does Cast<IMassWarUnitStateProvider>(Actor), so it's
 * AMassWarUnitAIController (the Actor registered with UMassWarUnitRegistrySubsystem) that implements the
 * interface, forwarding to this component's fields.
 */
UCLASS(ClassGroup = "MassWar", meta = (BlueprintSpawnableComponent))
class MASSWAREMBODIMENT_API UMassWarUnitStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "MassWar")
	uint8 TeamId = 0;

	UPROPERTY(EditAnywhere, Category = "MassWar")
	uint32 OwningPlayerId = 0;

	UPROPERTY(Transient)
	FMassWarOrderFragment Order;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
