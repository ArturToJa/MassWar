// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "Stats/MassWarStatTypes.h"
#include "MassWarStatSet.generated.h"

/**
 * A reusable list of stat changes, authored in the project: "Heavy armour: MoveSpeed x0.8, MaxHealth +50". Attach it to
 * a unit type with the MassWar Stats trait (weapons, ammo and upgrades will attach the same asset to themselves).
 */
UCLASS(BlueprintType)
class MASSWAR_API UMassWarStatSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (TitleProperty = "Stat"))
	TArray<FMassWarStatModifier> Modifiers;
};
