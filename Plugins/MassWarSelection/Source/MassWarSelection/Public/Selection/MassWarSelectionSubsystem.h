// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Subsystems/LocalPlayerSubsystem.h"
#include "MassEntityHandle.h"
#include "MassWarSelectionSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE(FMassWarOnSelectionChanged);

/**
 * Client-only, per-local-player current unit selection. Never replicated - only the orders issued
 * against a selection travel to the server (see UMassWarUnitOrderComponent). Works the same whether
 * MassWarReplication is installed or not.
 */
UCLASS()
class MASSWARSELECTION_API UMassWarSelectionSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	void SetSelection(const TArray<FMassEntityHandle>& NewSelection);
	void ClearSelection();

	const TArray<FMassEntityHandle>& GetSelection() const { return SelectedEntities; }
	bool HasSelection() const { return SelectedEntities.Num() > 0; }

	FMassWarOnSelectionChanged OnSelectionChanged;

private:
	UPROPERTY()
	TArray<FMassEntityHandle> SelectedEntities;
};
