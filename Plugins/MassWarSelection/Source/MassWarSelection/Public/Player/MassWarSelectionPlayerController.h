// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerController.h"
#include "MassEntityHandle.h"
#include "Player/MassWarOrderTarget.h"
#include "Interfaces/MassWarTeamProviderInterface.h"
#include "MassWarSelectionPlayerController.generated.h"

class UMassWarUnitOrderComponent;
struct FMassEntityManager;

/**
 * Client-only mouse selection + order input: left-click/drag selects units this player OWNS
 * (MassWarSelectionSubsystem; ownership is per-player, distinct from team - see
 * FMassWarOwnerFragment), right-click issues a Move order (empty ground) or Attack order (clicked an
 * enemy-team unit, regardless of who owns it) for the current selection via UMassWarUnitOrderComponent.
 * Implements IMassWarTeamProvider (Core) so Core-only-dependent plugins (e.g. MassWarFogOfWar) can learn
 * this player's team without depending on MassWarSelection.
 */
UCLASS()
class MASSWARSELECTION_API AMassWarSelectionPlayerController : public APlayerController, public IMassWarTeamProvider
{
	GENERATED_BODY()

public:
	AMassWarSelectionPlayerController();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	bool IsDragSelecting() const { return bIsDragging; }
	FVector2D GetDragStartScreenPos() const { return DragStartScreenPos; }
	FVector2D GetCurrentScreenPos() const { return CurrentScreenPos; }

	/** Server-only: which team this player selects/orders. GameMode assigns this once per connecting player. */
	void SetPlayerTeamId(uint8 InTeamId) { PlayerTeamId = InTeamId; }
	uint8 GetPlayerTeamId() const { return PlayerTeamId; }

	/** Server-only: this player's stable id, matching the FMassWarOwnerFragment on the units GameMode spawned for them. */
	void SetPlayerId(uint32 InPlayerId) { PlayerId = InPlayerId; }
	uint32 GetPlayerId() const { return PlayerId; }

	//~ IMassWarTeamProvider
	virtual uint8 GetMassWarPlayerTeamId() const override { return PlayerTeamId; }

protected:
	/** Which team this player selects/orders - assigned server-side by AMassWarDemoGameMode and replicated
	 *  down to this controller's own client; the EditDefaultsOnly default only matters before that happens
	 *  (e.g. no GameMode override) or in a non-networked test. */
	UPROPERTY(EditDefaultsOnly, Replicated, Category = "MassWar Selection")
	uint8 PlayerTeamId = 1;

	/** This player's stable id - matches FMassWarOwnerFragment::OwningPlayerId on the units GameMode
	 *  spawned for them. Assigned server-side and replicated, same as PlayerTeamId. 0 = unassigned. */
	UPROPERTY(Replicated)
	uint32 PlayerId = 0;

	/** Below this drag distance (pixels), a mouse-up is treated as a single click, not a box select. */
	UPROPERTY(EditDefaultsOnly, Category = "MassWar Selection")
	float DragThresholdPixels = 8.f;

	/** How close (pixels, in screen space) a unit's projected position must be to the cursor to count
	 *  as "clicked" for single-select or right-click targeting. */
	UPROPERTY(EditDefaultsOnly, Category = "MassWar Selection")
	float ClickPixelTolerance = 30.f;

	UPROPERTY()
	TObjectPtr<UMassWarUnitOrderComponent> OrderComponent;

private:
	void OnLeftClickPressed();
	void OnLeftClickReleased();
	void OnRightClickPressed();

	TArray<FMassEntityHandle> FindUnitsInScreenRect(const FVector2D& Min, const FVector2D& Max, bool bOwnedOnly) const;
	FMassEntityHandle FindNearestUnitAtScreenPos(const FVector2D& ScreenPos, float PixelRadius) const;
	FMassEntityManager* GetEntityManager() const;

	/** Selection is by formation: any selected unit selects every unit of its formation (units without one stay as they are). */
	TArray<FMassEntityHandle> ExpandToFormations(const TArray<FMassEntityHandle>& Units) const;

	/** Splits a selection into the distinct formation ids in it and the units that belong to no formation. */
	void CollectFormationIds(const TArray<FMassEntityHandle>& Units, TArray<int32>& OutFormationIds, TArray<FMassEntityHandle>& OutUnformedUnits) const;

	/** Bundles a local FMassEntityHandle with its Core FMassWarNetIdFragment (0 if unassigned/no Replication) for order RPCs - see FMassWarOrderTarget. */
	FMassWarOrderTarget MakeOrderTarget(FMassEntityManager& EntityManager, FMassEntityHandle Entity) const;
	TArray<FMassWarOrderTarget> MakeOrderTargets(FMassEntityManager& EntityManager, const TArray<FMassEntityHandle>& Entities) const;

	bool bIsLeftMouseDown = false;
	bool bIsDragging = false;
	FVector2D DragStartScreenPos = FVector2D::ZeroVector;
	FVector2D CurrentScreenPos = FVector2D::ZeroVector;
};
