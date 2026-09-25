// Copyright Epic Games, Inc. All Rights Reserved.

#include "Sight/MassWarLineOfSightSubsystem.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "CollisionQueryParams.h"

namespace
{
	TAutoConsoleVariable<int32> CVarRaycastsPerFrame(
		TEXT("MassWar.LOS.RaycastsPerFrame"), 400,
		TEXT("Line-of-sight raycasts the whole game may spend per frame (perception + fog of war share it)."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarEyeHeight(
		TEXT("MassWar.LOS.EyeHeight"), 100.f,
		TEXT("Height above a unit's position the sight ray starts at."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarTargetHeight(
		TEXT("MassWar.LOS.TargetHeight"), 60.f,
		TEXT("Height above a unit's position the sight ray aims at."),
		ECVF_Default);
}

bool UMassWarLineOfSightSubsystem::HasBudget()
{
	if (BudgetFrame != GFrameCounter)
	{
		BudgetFrame = GFrameCounter;
		UsedThisFrame = 0;
	}
	return UsedThisFrame < CVarRaycastsPerFrame.GetValueOnAnyThread();
}

bool UMassWarLineOfSightSubsystem::TraceLineOfSight(const FVector& ViewerLocation, const FVector& TargetLocation)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}
	HasBudget(); // rolls the frame counter over if needed
	++UsedThisFrame;

	const FVector Start = ViewerLocation + FVector(0.0, 0.0, CVarEyeHeight.GetValueOnGameThread());
	const FVector End = TargetLocation + FVector(0.0, 0.0, CVarTargetHeight.GetValueOnGameThread());
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MassWarLineOfSight), /*bTraceComplex=*/ false);
	return !World->LineTraceTestByChannel(Start, End, ECC_Visibility, Params);
}
