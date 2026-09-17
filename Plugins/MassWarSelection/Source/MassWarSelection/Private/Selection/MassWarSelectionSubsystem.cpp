// Copyright Epic Games, Inc. All Rights Reserved.

#include "Selection/MassWarSelectionSubsystem.h"

void UMassWarSelectionSubsystem::SetSelection(const TArray<FMassEntityHandle>& NewSelection)
{
	SelectedEntities = NewSelection;
	OnSelectionChanged.Broadcast();
}

void UMassWarSelectionSubsystem::ClearSelection()
{
	if (SelectedEntities.Num() > 0)
	{
		SelectedEntities.Reset();
		OnSelectionChanged.Broadcast();
	}
}
