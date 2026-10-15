// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "Spawning/MassWarSpawnTypes.h"
#include "MassWarFormationTemplate.generated.h"

/**
 * A formation described as data: "20 riflemen with M4s, 5 grenadiers with RPGs, loose blob". Authored by designers in
 * the project (the plugin ships none) and spawned with UMassWarSpawnSubsystem::SpawnFormationFromTemplate, so a level
 * script or game mode only says where, for whom, and which template.
 */
UCLASS(BlueprintType)
class MASSWARSPAWNING_API UMassWarFormationTemplate : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** The units the formation owns. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Formation")
	TArray<FMassWarUnitSpawnEntry> Units;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Formation")
	FMassWarFormationSettings Settings;

	/** The spawn parameters for this template at a place, for a player and team. */
	FMassWarFormationSpawnParams MakeParams(int32 OwningPlayerId, int32 TeamId, const FVector& Location, float YawDegrees = 0.f) const;
};
