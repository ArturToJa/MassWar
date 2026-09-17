// Copyright Epic Games, Inc. All Rights Reserved.

#include "Controllers/MassWarUnitAIController.h"
#include "Components/StateTreeAIComponent.h"
#include "UnitBrain/MassWarUnitStateComponent.h"
#include "StateTree.h"
#include "StateTreeReference.h"
#include "UObject/UObjectGlobals.h"

AMassWarUnitAIController::AMassWarUnitAIController()
{
	StateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAIComponent"));
	StateComponent = CreateDefaultSubobject<UMassWarUnitStateComponent>(TEXT("StateComponent"));

	// Deliberately not wired here: a ConstructorHelpers::FObjectFinder load at CDO-construction time
	// (i.e. at module load, before StateTreeEditorModule has bound its compile-on-load delegates) made
	// UStateTree::PostLoad's CompileIfChanged() treat the asset as uncompilable ("could not compile,
	// please resave") and reset it - see BeginPlay, which loads it late enough for that module to be up.
}

void AMassWarUnitAIController::BeginPlay()
{
	// UStateTreeComponent::BeginPlay() (run as part of Super::BeginPlay(), which cascades to owned
	// components) calls StartLogic() automatically if bStartLogicAutomatically - but StateTreeRef isn't
	// set until after that, so it no-ops on an invalid asset. SetStateTreeReference below only validates,
	// it doesn't (re)start the tree, hence the explicit StartLogic() call afterward.
	Super::BeginPlay();

	if (StateTreeAIComponent)
	{
		// Built by MassWarSetupEmbodimentCommandlet (-run=MassWarSetupEmbodiment); only succeeds once
		// that commandlet has been run at least once.
		if (UStateTree* Asset = LoadObject<UStateTree>(nullptr, TEXT("/Game/MassWar/ST_MassWarUnit_Actor.ST_MassWarUnit_Actor")))
		{
			FStateTreeReference Ref;
			Ref.SetStateTree(Asset);
			StateTreeAIComponent->SetStateTreeReference(Ref);
			StateTreeAIComponent->StartLogic();
		}
	}
}

uint8 AMassWarUnitAIController::GetMassWarTeamId() const
{
	return StateComponent ? StateComponent->TeamId : 0;
}

FVector AMassWarUnitAIController::GetMassWarLocation() const
{
	const APawn* ControlledPawn = GetPawn();
	return ControlledPawn ? ControlledPawn->GetActorLocation() : FVector::ZeroVector;
}

const FMassWarOrderFragment* AMassWarUnitAIController::GetMassWarOrder() const
{
	return StateComponent ? &StateComponent->Order : nullptr;
}

FMassWarOrderFragment* AMassWarUnitAIController::GetMassWarOrderMutable()
{
	return StateComponent ? &StateComponent->Order : nullptr;
}
