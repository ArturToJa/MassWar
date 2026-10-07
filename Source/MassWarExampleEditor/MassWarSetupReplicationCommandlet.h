// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MassWarSetupReplicationCommandlet.generated.h"

/**
 * One-off setup tool (run via `UnrealEditor-Cmd.exe <uproject> -run=MassWarSetupReplication`): adds a
 * UMassReplicationTrait to DA_MassWarDemoUnit and points it at MassWarReplication's bubble/replicator
 * classes. Editor-asset equivalent of MassWarSetupStateTreeCommandlet - a data asset's Traits array is a
 * plain EditAnywhere/Instanced UPROPERTY, so this is a much smaller script than the StateTree one.
 * Project-specific demo tooling, not part of any MassWar plugin.
 */
UCLASS()
class UMassWarSetupReplicationCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
