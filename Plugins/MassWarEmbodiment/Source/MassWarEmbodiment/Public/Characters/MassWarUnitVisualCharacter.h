// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Character.h"
#include "MassWarUnitVisualCharacter.generated.h"

class UStaticMeshComponent;

/**
 * Cosmetic-only near-LOD puppet for ordinary Mass units - configured as an ordinary Mass visualization
 * trait's HighResTemplateActor (see MassWarSetupEmbodimentCommandlet), so Mass's own
 * MassRepresentationActorManagement spawns/despawns and position/rotation-syncs it automatically when
 * the camera gets close; this class supplies zero simulation authority of its own (no AI controller, no
 * Order/state data - it never drives gameplay, only what's rendered).
 *
 * Visual: see AMassWarUnitCharacter's header comment - the stock mannequin skeleton/animations weren't
 * available as a plain importable asset here, so this uses the same plain primitive body mesh as the
 * hero Character, per the Pass 7 plan's documented fallback. Swap in real (skinned, VAT-baked) art later
 * without touching C++.
 */
UCLASS()
class MASSWAREMBODIMENT_API AMassWarUnitVisualCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMassWarUnitVisualCharacter();

protected:
	UPROPERTY(VisibleAnywhere, Category = "MassWar|Visual")
	TObjectPtr<UStaticMeshComponent> BodyMesh;
};
