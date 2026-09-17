// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/HUD.h"
#include "MassWarHUD.generated.h"

/** Draws the live marquee selection box and a ring under each currently selected unit. */
UCLASS()
class MASSWARSELECTION_API AMassWarHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "MassWar Selection")
	FLinearColor MarqueeColor = FLinearColor(0.2f, 1.f, 0.3f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "MassWar Selection")
	FLinearColor SelectionRingColor = FLinearColor(0.2f, 1.f, 0.3f, 1.f);

	/** Screen-space (pixel) size of the diamond marker drawn under each selected unit. */
	UPROPERTY(EditDefaultsOnly, Category = "MassWar Selection")
	float SelectionMarkerSizePixels = 14.f;

private:
	void DrawMarquee();
	void DrawSelectionRings();
};
