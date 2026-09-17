// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityTraitBase.h"
#include "MassWarReplicationSmoothingTrait.generated.h"

/**
 * Optional add-on alongside UMassReplicationTrait: adds FMassWarClientInterpolationFragment so a
 * replicated unit's position/rotation are smoothly chased on the client instead of hard-snapped on
 * every replication update. Only affects client-spawned proxy entities - the fragment sits unused on
 * server/standalone entities (nothing ever reads it there).
 */
UCLASS(meta = (DisplayName = "MassWar Replication Client Smoothing"))
class MASSWARREPLICATION_API UMassWarReplicationSmoothingTrait : public UMassEntityTraitBase
{
	GENERATED_BODY()

public:
	/** Higher = snaps toward each new replicated position/rotation faster (less smoothing lag). */
	UPROPERTY(EditAnywhere, Category = "MassWar|Replication")
	float InterpSpeed = 8.f;

protected:
	virtual void BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const override;
};
