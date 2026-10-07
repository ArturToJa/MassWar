// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "Perception/MassWarPerceptionSubsystem.h"
#include "MassWarPerceptionProcessor.generated.h"

/**
 * Runs every unit's senses and keeps their memory tidy. Server/Standalone only.
 *  - Sight: each unit scans on its own staggered timer (SightUpdateInterval), asking a shared hashed grid of all
 *    living units for enemies within its sight radius and vision cone; it keeps the nearest few.
 *  - Hearing: noises queued in UMassWarPerceptionSubsystem are resolved against the same grid.
 *  - Memory: perceived units that are dead or older than MemoryDuration are forgotten.
 * Damage stimuli are written straight into the victim by the subsystem (combat runs before this processor).
 * The grid is rebuilt every GridRebuildInterval seconds, so targets are seen at most that far out of date.
 */
UCLASS()
class MASSWARPERCEPTION_API UMassWarPerceptionProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassWarPerceptionProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	/** Width of one grid cell (uu). Around a typical sight radius / 3 works well. */
	UPROPERTY(EditAnywhere, Category = "MassWar|Perception", meta = (ClampMin = "100.0"))
	float GridCellSize = 1000.f;

	/** Seconds between rebuilds of the target grid. */
	UPROPERTY(EditAnywhere, Category = "MassWar|Perception", meta = (ClampMin = "0.0"))
	float GridRebuildInterval = 0.1f;

private:
	void RebuildGrid(FMassExecutionContext& Context);
	void ProcessNoises(FMassEntityManager& EntityManager, double Now);

	/** Calls Func(gridIndex) for every grid unit within Radius (2D) of Center. */
	template<typename FuncType>
	void ForEachInRadius(const FVector3f& Center, float Radius, FuncType&& Func) const;

	FMassEntityQuery TargetQuery;
	FMassEntityQuery ObserverQuery;

	double NextGridBuildTime = 0.0;
	float InvCellSize = 1.f;
	uint32 HashMask = 0;

	TArray<FMassEntityHandle> GridEntities;
	TArray<FVector3f> GridPositions;
	TArray<uint8> GridTeams;
	TArray<int32> GridCellX;
	TArray<int32> GridCellY;
	TArray<int32> GridNext;
	TArray<int32> GridHead;

	TArray<FMassWarNoiseEvent> Noises;
};
