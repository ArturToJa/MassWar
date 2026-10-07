// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "MassEntityHandle.h"
#include "MassWarGhostSubsystem.generated.h"

/** Where an enemy unit was last seen, after it slipped out of the local player's sight. */
USTRUCT(BlueprintType)
struct MASSWARFOGOFWAR_API FMassWarGhost
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "MassWar|FogOfWar")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "MassWar|FogOfWar")
	float YawDegrees = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "MassWar|FogOfWar")
	uint8 TeamId = 0;

	/** Seconds since the unit was lost from sight. */
	UPROPERTY(BlueprintReadOnly, Category = "MassWar|FogOfWar")
	float Age = 0.f;

	/** Remote client: the unit's replicated id. Listen server / standalone: 0 (Entity is used instead). */
	uint32 NetId = 0;
	FMassEntityHandle Entity;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMassWarGhostEvent, const FMassWarGhost&, Ghost);

/**
 * "Last seen" memory for the local player: when an enemy that was in sight goes out of sight (still alive), a ghost
 * keeps its last known position and heading until the enemy is seen again or GhostLifetime runs out. It only ever
 * holds what the fog of war had already revealed to this player - never new information.
 *  - Remote client: fed by MassWarReplication's client-side "agent left the bubble" notification.
 *  - Listen server / standalone: fed by UMassWarVisibilitySubsystem when the local team stops seeing a unit.
 * The plugin ships no marker content: bind OnGhostAdded/OnGhostRemoved (or read GetGhosts) to show your own marker,
 * or turn on the built-in debug marker with MassWar.Ghosts.Draw 1.
 */
UCLASS()
class MASSWARFOGOFWAR_API UMassWarGhostSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void PostInitialize() override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UMassWarGhostSubsystem, STATGROUP_Tickables); }

	const TArray<FMassWarGhost>& GetGhosts() const { return Ghosts; }

	/** Server-side world (listen server / standalone): the local team just lost sight of Entity. */
	void AddGhostForEntity(FMassEntityHandle Entity, const FVector& Location, float YawDegrees, uint8 TeamId);
	void ClearGhostForEntity(FMassEntityHandle Entity);

	/** The local player's team (0 if there is no local player, e.g. a dedicated server). */
	uint8 GetLocalTeamId() const;

	UPROPERTY(BlueprintAssignable, Category = "MassWar|FogOfWar")
	FMassWarGhostEvent OnGhostAdded;

	UPROPERTY(BlueprintAssignable, Category = "MassWar|FogOfWar")
	FMassWarGhostEvent OnGhostRemoved;

private:
	void HandleClientAgentRemoved(uint32 NetId, const FVector& Location, float YawDegrees, uint8 TeamId, uint8 LifeState);
	void HandleClientAgentAdded(uint32 NetId);
	void AddGhost(const FMassWarGhost& Ghost);
	void RemoveGhostAt(int32 Index);

	TArray<FMassWarGhost> Ghosts;
	FDelegateHandle RemovedHandle;
	FDelegateHandle AddedHandle;
};
