// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MassWarHUD.h"
#include "Player/MassWarSelectionPlayerController.h"
#include "Selection/MassWarSelectionSubsystem.h"
#include "Registry/MassWarUnitRegistrySubsystem.h"
#include "MassSpawnerSubsystem.h"
#include "MassEntityManager.h"
#include "MassCommonFragments.h"
#include "UnitBrain/MassWarUnitStateView.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/LocalPlayer.h"

void AMassWarHUD::DrawHUD()
{
	Super::DrawHUD();

	DrawMarquee();
	DrawSelectionRings();
}

void AMassWarHUD::DrawMarquee()
{
	const AMassWarSelectionPlayerController* PC = Cast<AMassWarSelectionPlayerController>(GetOwningPlayerController());
	if (!PC || !PC->IsDragSelecting())
	{
		return;
	}

	const FVector2D A = PC->GetDragStartScreenPos();
	const FVector2D B = PC->GetCurrentScreenPos();
	const FVector2D Min(FMath::Min(A.X, B.X), FMath::Min(A.Y, B.Y));
	const FVector2D Max(FMath::Max(A.X, B.X), FMath::Max(A.Y, B.Y));

	DrawLine(Min.X, Min.Y, Max.X, Min.Y, MarqueeColor);
	DrawLine(Max.X, Min.Y, Max.X, Max.Y, MarqueeColor);
	DrawLine(Max.X, Max.Y, Min.X, Max.Y, MarqueeColor);
	DrawLine(Min.X, Max.Y, Min.X, Min.Y, MarqueeColor);
}

void AMassWarHUD::DrawSelectionRings()
{
	const APlayerController* PC = GetOwningPlayerController();
	const ULocalPlayer* LP = PC ? PC->GetLocalPlayer() : nullptr;
	const UMassWarSelectionSubsystem* SelectionSubsystem = LP ? LP->GetSubsystem<UMassWarSelectionSubsystem>() : nullptr;
	if (!SelectionSubsystem || !SelectionSubsystem->HasSelection())
	{
		return;
	}

	UMassSpawnerSubsystem* Spawner = GetWorld() ? GetWorld()->GetSubsystem<UMassSpawnerSubsystem>() : nullptr;
	if (!Spawner)
	{
		return;
	}

	FMassEntityManager& EntityManager = Spawner->GetEntityManagerChecked();
	const float S = SelectionMarkerSizePixels;

	for (const FMassEntityHandle& Entity : SelectionSubsystem->GetSelection())
	{
		if (!FMassWarUnitStateView::IsLiving(EntityManager, Entity))
		{
			continue;
		}

		const FTransformFragment* Transform = EntityManager.GetFragmentDataPtr<FTransformFragment>(Entity);
		if (!Transform)
		{
			continue;
		}

		FVector2D ScreenPos;
		if (!UGameplayStatics::ProjectWorldToScreen(PC, Transform->GetTransform().GetLocation(), ScreenPos))
		{
			continue;
		}

		// Diamond marker centered on the unit's projected screen position.
		const FVector2D Top(ScreenPos.X, ScreenPos.Y - S);
		const FVector2D Bottom(ScreenPos.X, ScreenPos.Y + S);
		const FVector2D Left(ScreenPos.X - S, ScreenPos.Y);
		const FVector2D Right(ScreenPos.X + S, ScreenPos.Y);

		DrawLine(Top.X, Top.Y, Right.X, Right.Y, SelectionRingColor, 2.f);
		DrawLine(Right.X, Right.Y, Bottom.X, Bottom.Y, SelectionRingColor, 2.f);
		DrawLine(Bottom.X, Bottom.Y, Left.X, Left.Y, SelectionRingColor, 2.f);
		DrawLine(Left.X, Left.Y, Top.X, Top.Y, SelectionRingColor, 2.f);
	}
}
