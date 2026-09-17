// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MassWarSetupStateTreeCommandlet.generated.h"

/**
 * One-off setup tool (run via `UnrealEditor-Cmd.exe <uproject> -run=MassWarSetupStateTree`): builds and
 * compiles the ST_MassWarUnit_Mass StateTree asset in C++ using StateTree's own "builder API"
 * (UStateTreeEditorData::AddSubTree/AddEvaluator, UStateTreeState::AddTask/AddEnterCondition/
 * AddTransition) rather than trying to script the graph editor - this is the same API Epic's own
 * StateTree test suite uses to build trees programmatically, so it's a proven, compiler-checked path
 * instead of guessing at editor-only Python bindings for a notoriously hard-to-script graph asset.
 * Project-specific demo tooling, not part of any MassWar plugin.
 */
UCLASS()
class UMassWarSetupStateTreeCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
