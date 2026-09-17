// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MassWarSetupEmbodimentCommandlet.generated.h"

/**
 * One-off setup tool (run via `UnrealEditor-Cmd.exe <uproject> -run=MassWarSetupEmbodiment`): builds and
 * compiles the ST_MassWarUnit_Actor StateTree asset - the Actor-schema counterpart to
 * ST_MassWarUnit_Mass, same 3-state Idle/MoveToRange/Attack graph, driving MassWarEmbodiment's embodied
 * units (AMassWarUnitCharacter via AMassWarUnitAIController) instead of Mass entities. Mirrors
 * MassWarSetupStateTreeCommandlet exactly, just with UStateTreeComponentSchema and the Actor-schema node
 * types. Project-specific demo tooling, not part of any MassWar plugin.
 */
UCLASS()
class UMassWarSetupEmbodimentCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
