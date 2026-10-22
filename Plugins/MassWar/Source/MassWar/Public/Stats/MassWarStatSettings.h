// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "Stats/MassWarStatTypes.h"
#include "Stats/MassWarStatSet.h"
#include "MassWarStatSettings.generated.h"

/**
 * The project's own stats, on top of the built-in ones (EMassWarStat). Order matters and is append-only: a stat's
 * place in this list is its id, so insert new stats at the end and do not remove or reorder existing ones.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "MassWar Stats"))
class MASSWAR_API UMassWarStatSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	UPROPERTY(EditAnywhere, Config, Category = "Stats", meta = (TitleProperty = "Tag"))
	TArray<FMassWarStatDefinition> CustomStats;

	/**
	 * Stat sets that can be applied to units while the game runs (upgrades, buffs, ...) and replicated: a unit tells clients
	 * which of these it has by id (position + 1, at most 255), and every client derives the same numbers. Append only - the
	 * position is the id, so do not remove or reorder entries. Stat sets that are only baked into a unit type (the MassWar
	 * Stats trait) do not need to be listed.
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Stats")
	TArray<TSoftObjectPtr<UMassWarStatSet>> StatSetCatalog;
};
