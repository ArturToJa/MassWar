// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Formation/MassWarFormationTypes.h"

/** Slot geometry for the formation shapes. Local frame: +X is the direction the formation faces / travels,
 *  +Y is to its right. Slot 0 is the front-centre; later slots go outwards/backwards. */
namespace MassWarFormationLayout
{
	MASSWARFORMATIONS_API void ComputeSlotOffsets(const FMassWarFormationSettings& Settings, int32 Count, TArray<FVector2D>& OutOffsets);

	/** Half of the formation's sideways extent - used to set several formations next to each other. */
	MASSWARFORMATIONS_API float ComputeHalfWidth(const FMassWarFormationSettings& Settings, int32 Count);
}
