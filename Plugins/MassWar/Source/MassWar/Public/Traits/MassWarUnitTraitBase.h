// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityTraitBase.h"
#include "MassWarUnitTraitBase.generated.h"

/**
 * Base trait for every MassWar unit. Adds the Core-owned identity fragments (Team, Order, stable
 * network id) that every other MassWar plugin builds on. Combine with an engine movement trait and an
 * engine representation trait on the same UMassEntityConfigAsset to get a spawnable unit type; add
 * MassWarCombat/MassWarStateTreeAI/MassWarReplication traits later to layer in more behavior.
 */
UCLASS(meta = (DisplayName = "MassWar Unit"))
class MASSWAR_API UMassWarUnitTraitBase : public UMassEntityTraitBase
{
	GENERATED_BODY()

public:
	/** Starting team id for units spawned from this config. 0 = neutral/unassigned. */
	UPROPERTY(EditAnywhere, Category = "MassWar")
	uint8 DefaultTeamId = 1;

	UPROPERTY(EditAnywhere, Category = "MassWar|Movement")
	float MoveSpeed = 500.f;

	UPROPERTY(EditAnywhere, Category = "MassWar|Movement")
	float AcceptanceRadius = 50.f;

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
