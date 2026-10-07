// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "MassWarSetupAttackAnimCommandlet.generated.h"

/**
 * One-off setup tool (run via `UnrealEditor-Cmd.exe <uproject> -run=MassWarSetupAttackAnim`): turns the
 * mannequin's attack animation sequences into montages (the puppet plays a montage per attack, through the
 * anim blueprint's DefaultSlot) and assigns them to BP_MassWarUnitVisual's AttackMontages. Safe to re-run:
 * montages that already exist are reused, not recreated. Project-specific demo tooling, not part of any
 * MassWar plugin - the plugin ships no animation content.
 */
UCLASS()
class UMassWarSetupAttackAnimCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
