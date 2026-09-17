// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MassEntityTypes.h"
#include "MassEntityHandle.h"
#include "MassWarDemoGameMode.generated.h"

class UMassEntityConfigAsset;
struct IConsoleCommand;

/**
 * Test-harness GameMode for the MassWar plugin suite demo project. Not part of any plugin - this is
 * throwaway scaffolding specific to this example project. Spawns 150 units for each connecting player
 * (owned by that player, on the team AssignTeamForPlayer() gives them) as they join, rather than a
 * fixed pre-spawn - see HandleStartingNewPlayer_Implementation.
 */
UCLASS()
class MASSWAREXAMPLE_API AMassWarDemoGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMassWarDemoGameMode();

	virtual void BeginPlay() override;

	/** Assigns this player their id/team, spawns their 150 units, and centers the RTS camera pawn over
	 *  the spawn area (units spawn wherever their computed origin says, regardless of the level's own
	 *  PlayerStart). */
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	/** Entity config asset every player's units are spawned from; team/owner are assigned per-entity after spawning. */
	UPROPERTY(EditDefaultsOnly, Category = "MassWar Demo")
	FSoftObjectPath DemoUnitConfigPath;

	UPROPERTY(EditDefaultsOnly, Category = "MassWar Demo")
	int32 UnitsPerPlayer = 150;

	UPROPERTY(EditDefaultsOnly, Category = "MassWar Demo")
	float TeamOriginOffset = 4000.f;

	UPROPERTY(EditDefaultsOnly, Category = "MassWar Demo")
	float SpawnAreaExtent = 1500.f;

	/** Height and downward pitch of the bird's-eye demo camera placed over the spawn area. Kept well
	 *  inside Mass's default LOD distance bands (Low LOD tops out at 10000-15000 units by default) so
	 *  distance-based LOD culling can't be the reason units are invisible. */
	UPROPERTY(EditDefaultsOnly, Category = "MassWar Demo")
	float DemoCameraHeight = 1500.f;

private:
	/** Which team the Nth connecting player (1-based join order) is put on. Change this one function to
	 *  re-group players differently later - e.g. `return ((PlayerIndex - 1) / 2) + 1;` would pair players
	 *  up two-at-a-time onto shared teams instead of everyone getting their own. */
	static uint8 AssignTeamForPlayer(int32 PlayerIndex);

	/** Where the Nth connecting player's units spawn - alternates left/right of the map center, stepping
	 *  further out for every second player so slots never overlap. */
	FVector ComputeSpawnOriginForPlayer(int32 PlayerIndex) const;

	UMassEntityConfigAsset* GetOrLoadDemoUnitConfig();

	void SpawnUnitsForPlayer(UMassEntityConfigAsset& Config, uint32 PlayerId, uint8 TeamId, const FVector& Origin, int32 Count);

	/** Pass 7 (MassWarEmbodiment) test harness: spawns one AMassWarUnitCharacter "hero" among the first
	 *  connecting player's Mass units, gated to only that first player so the test scenario stays simple
	 *  (one hero to observe, not one per player) - out of scope for selection/replication/fog-of-war. */
	void SpawnHeroForPlayer(uint8 TeamId, uint32 PlayerId, const FVector& Origin);

	UPROPERTY(EditDefaultsOnly, Category = "MassWar Demo")
	TSubclassOf<APawn> HeroCharacterClass;

	bool bHeroSpawned = false;

	/** Dumps LOD/representation/transform state for a handful of spawned entities a couple seconds
	 *  after spawn, to see directly (via log) whether Mass ever considers them renderable, instead of
	 *  guessing further from the outside. Debug-only, not meant to stay long-term. */
	void LogEntityDiagnostics();

	UPROPERTY()
	TArray<FMassEntityHandle> DebugSampleEntities;

	FTimerHandle DebugDiagnosticsTimerHandle;

	/** Pass 2 (MassWarCombat) test harness: "MassWar.DebugAttack" orders every unit on every team to
	 *  attack its nearest enemy-team unit, so combat can be tested before Selection/StateTreeAI exist
	 *  to issue orders a more normal way. */
	void DebugAttackNearestEnemy();
	void UpdateDebugHUD();

	/** Pass 3 (MassWarSelection) test harness: "MassWar.DebugMove" orders the lowest-numbered team's
	 *  units to move toward another team's units, to self-verify MassWarOrderMovementProcessor without
	 *  needing mouse input. */
	void DebugMoveFirstTeamTowardOthers();

	IConsoleCommand* DebugAttackConsoleCommand = nullptr;
	IConsoleCommand* DebugMoveConsoleCommand = nullptr;
	FTimerHandle DebugHUDTimerHandle;

	UPROPERTY()
	TObjectPtr<UMassEntityConfigAsset> CachedDemoUnitConfig;

	/** Every spawned entity, grouped by team - multiple players (and, later, bots) can share a team. */
	TMap<uint8, TArray<FMassEntityHandle>> EntitiesByTeam;

	/** 1-based; the Nth player to connect this session. */
	int32 NextPlayerIndex = 1;

	/** Stable per-player id matching FMassWarOwnerFragment::OwningPlayerId - 0 is reserved for "unowned". */
	uint32 NextPlayerId = 1;
};
