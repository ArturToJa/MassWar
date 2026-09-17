// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AIController.h"
#include "Interfaces/MassWarUnitStateProviderInterface.h"
#include "MassWarUnitAIController.generated.h"

class UMassWarUnitStateComponent;
class UStateTreeAIComponent;

/**
 * Bare AIController that drives an embodied MassWar unit via ST_MassWarUnit_Actor. Owns both the
 * UStateTreeAIComponent (StateTree is "designed to be run on an AIController" per its own class comment)
 * and the UMassWarUnitStateComponent the Actor-schema StateTree nodes bind to -
 * UStateTreeComponentSchema::CollectExternalData resolves a UActorComponent-typed external data handle
 * via FindComponentByClass on the StateTree component's own Owner, which is this controller, not the
 * possessed Pawn.
 *
 * Implements IMassWarUnitStateProvider (forwarding to UMassWarUnitStateComponent) so this controller -
 * the Actor registered with UMassWarUnitRegistrySubsystem::RegisterActorUnit - is what
 * FMassWarUnitStateView::FromHandle's Cast<IMassWarUnitStateProvider>(Actor) resolves.
 */
UCLASS()
class MASSWAREMBODIMENT_API AMassWarUnitAIController : public AAIController, public IMassWarUnitStateProvider
{
	GENERATED_BODY()

public:
	AMassWarUnitAIController();

	UMassWarUnitStateComponent* GetStateComponent() const { return StateComponent; }

	//~ IMassWarUnitStateProvider
	virtual uint8 GetMassWarTeamId() const override;
	virtual FVector GetMassWarLocation() const override;
	virtual const FMassWarOrderFragment* GetMassWarOrder() const override;
	virtual FMassWarOrderFragment* GetMassWarOrderMutable() override;

protected:
	virtual void BeginPlay() override;


	UPROPERTY(VisibleAnywhere, Category = "MassWar")
	TObjectPtr<UStateTreeAIComponent> StateTreeAIComponent;

	UPROPERTY(VisibleAnywhere, Category = "MassWar")
	TObjectPtr<UMassWarUnitStateComponent> StateComponent;
};
