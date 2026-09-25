// Copyright Epic Games, Inc. All Rights Reserved.

#include "Visibility/MassWarVisibilityProcessor.h"
#include "Visibility/MassWarVisibilitySubsystem.h"
#include "Fragments/MassWarVisibilityFragment.h"
#include "Fragments/MassWarUnitFragments.h"
#include "Sight/MassWarLineOfSightSubsystem.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "Engine/World.h"
#include "Algo/Sort.h"

namespace
{
	FORCEINLINE uint32 CellHash(int32 X, int32 Y)
	{
		return static_cast<uint32>(X) * 73856093u ^ static_cast<uint32>(Y) * 19349663u;
	}

	/** One team's units in a hashed uniform grid (buckets may merge cells; the stored cell coordinates keep
	 *  lookups exact). One grid per team so a candidate only ever searches the ENEMY viewers - a single shared grid
	 *  made every lookup wade through the candidate's own (dense) team. */
	struct FTeamGrid
	{
		uint8 TeamId = 0;
		TArray<int32> Units; // indices into the unit list
		TArray<int32> Head, Next, CellX, CellY;
		uint32 Mask = 0;
		FVector2D BoundsMin = FVector2D(TNumericLimits<double>::Max(), TNumericLimits<double>::Max());
		FVector2D BoundsMax = FVector2D(-TNumericLimits<double>::Max(), -TNumericLimits<double>::Max());
	};
}

UMassWarVisibilityProcessor::UMassWarVisibilityProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);

	// Touches World::GetSubsystem<>() and mutates UMassWarVisibilitySubsystem - see
	// UMassWarLocalVisibilityProcessor's constructor comment for why this can't run off the game thread.
	// Line-of-sight raycasts are game-thread-only as well.
	bRequiresGameThreadExecution = true;
}

void UMassWarVisibilityProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassWarTeamFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassWarVisibilityFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassWarLifeFragment>(EMassFragmentAccess::ReadOnly);
}

void UMassWarVisibilityProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	TimeSinceLastUpdate += Context.GetDeltaTimeSeconds();
	if (TimeSinceLastUpdate < UpdateInterval)
	{
		return;
	}
	TimeSinceLastUpdate = 0.f;

	UWorld* World = EntityManager.GetWorld();
	UMassWarVisibilitySubsystem* VisibilitySubsystem = World ? World->GetSubsystem<UMassWarVisibilitySubsystem>() : nullptr;
	if (!VisibilitySubsystem)
	{
		return;
	}
	UMassWarLineOfSightSubsystem* LineOfSight = bRequireLineOfSight ? World->GetSubsystem<UMassWarLineOfSightSubsystem>() : nullptr;
	const double Now = World->GetTimeSeconds();

	struct FUnitInfo
	{
		FMassEntityHandle Entity;
		uint8 TeamId = 0;
		FVector Location = FVector::ZeroVector;
		float SightRadius = 0.f;
	};

	// A dying unit neither sees nor is seen: it is out of the game already.
	TArray<FUnitInfo> Units;
	float MaxSight = 0.f;
	EntityQuery.ForEachEntityChunk(Context, [&Units, &MaxSight](FMassExecutionContext& Context)
	{
		const TConstArrayView<FMassWarTeamFragment> TeamList = Context.GetFragmentView<FMassWarTeamFragment>();
		const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FMassWarVisibilityFragment> VisibilityList = Context.GetFragmentView<FMassWarVisibilityFragment>();
		const TConstArrayView<FMassWarLifeFragment> LifeList = Context.GetFragmentView<FMassWarLifeFragment>();

		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			if (LifeList[It].IsDying() || TeamList[It].TeamId == 0)
			{
				continue;
			}
			FUnitInfo& Unit = Units.AddDefaulted_GetRef();
			Unit.Entity = Context.GetEntity(It);
			Unit.TeamId = TeamList[It].TeamId;
			Unit.Location = TransformList[It].GetTransform().GetLocation();
			Unit.SightRadius = VisibilityList[It].SightRadius;
			MaxSight = FMath::Max(MaxSight, Unit.SightRadius);
		}
	});

	// One grid per team; every present team gets an (initially empty) visible set.
	TMap<uint8, TSet<FMassEntityHandle>> NewVisibility;
	TArray<FTeamGrid> Grids;
	const float InvCellSize = 1.f / GridCellSize;
	for (int32 Index = 0; Index < Units.Num(); ++Index)
	{
		const uint8 TeamId = Units[Index].TeamId;
		FTeamGrid* Grid = Grids.FindByPredicate([TeamId](const FTeamGrid& G) { return G.TeamId == TeamId; });
		if (!Grid)
		{
			Grid = &Grids.AddDefaulted_GetRef();
			Grid->TeamId = TeamId;
			NewVisibility.Add(TeamId);
		}
		Grid->Units.Add(Index);
	}
	for (FTeamGrid& Grid : Grids)
	{
		const int32 Count = Grid.Units.Num();
		Grid.Mask = FMath::RoundUpToPowerOfTwo(static_cast<uint32>(Count) * 2u + 16u) - 1u;
		Grid.Head.Init(INDEX_NONE, Grid.Mask + 1u);
		Grid.Next.SetNumUninitialized(Count);
		Grid.CellX.SetNumUninitialized(Count);
		Grid.CellY.SetNumUninitialized(Count);
		for (int32 Local = 0; Local < Count; ++Local)
		{
			const FVector& Location = Units[Grid.Units[Local]].Location;
			Grid.BoundsMin.X = FMath::Min(Grid.BoundsMin.X, Location.X);
			Grid.BoundsMin.Y = FMath::Min(Grid.BoundsMin.Y, Location.Y);
			Grid.BoundsMax.X = FMath::Max(Grid.BoundsMax.X, Location.X);
			Grid.BoundsMax.Y = FMath::Max(Grid.BoundsMax.Y, Location.Y);
			Grid.CellX[Local] = FMath::FloorToInt(Location.X * InvCellSize);
			Grid.CellY[Local] = FMath::FloorToInt(Location.Y * InvCellSize);
			const uint32 Bucket = CellHash(Grid.CellX[Local], Grid.CellY[Local]) & Grid.Mask;
			Grid.Next[Local] = Grid.Head[Bucket];
			Grid.Head[Bucket] = Local;
		}
	}

	if (Units.IsEmpty() || MaxSight <= 0.f)
	{
		VisibilitySubsystem->SetTeamVisibility(MoveTemp(NewVisibility));
		return;
	}

	const int32 TestsPerUnit = bRequireLineOfSight ? FMath::Max(1, MaxLineOfSightTestsPerUnit) : 1;
	const int32 MaxViewersToCollect = TestsPerUnit * 3;
	struct FViewer
	{
		int32 Index;
		double DistSq;
	};

	for (const FUnitInfo& Candidate : Units)
	{
		const int32 MinX = FMath::FloorToInt((Candidate.Location.X - MaxSight) * InvCellSize);
		const int32 MaxX = FMath::FloorToInt((Candidate.Location.X + MaxSight) * InvCellSize);
		const int32 MinY = FMath::FloorToInt((Candidate.Location.Y - MaxSight) * InvCellSize);
		const int32 MaxY = FMath::FloorToInt((Candidate.Location.Y + MaxSight) * InvCellSize);
		const int64 CellsToVisit = static_cast<int64>(MaxX - MinX + 1) * (MaxY - MinY + 1);

		for (const FTeamGrid& Grid : Grids)
		{
			if (Grid.TeamId == Candidate.TeamId)
			{
				continue;
			}
			// Nothing of this team within sight range of the candidate's position at all: skip the lookup.
			if (Candidate.Location.X < Grid.BoundsMin.X - MaxSight || Candidate.Location.X > Grid.BoundsMax.X + MaxSight
				|| Candidate.Location.Y < Grid.BoundsMin.Y - MaxSight || Candidate.Location.Y > Grid.BoundsMax.Y + MaxSight)
			{
				continue;
			}

			// The first few units of this (enemy) team that have the candidate within their sight radius.
			TArray<FViewer, TInlineAllocator<16>> Viewers;
			auto Consider = [&](int32 Local)
			{
				const int32 Other = Grid.Units[Local];
				const FUnitInfo& Viewer = Units[Other];
				const double DistSq = FVector::DistSquared(Viewer.Location, Candidate.Location);
				if (DistSq <= FMath::Square(static_cast<double>(Viewer.SightRadius)))
				{
					Viewers.Add({ Other, DistSq });
				}
			};
			if (CellsToVisit > Grid.Units.Num())
			{
				for (int32 Local = 0; Local < Grid.Units.Num() && Viewers.Num() < MaxViewersToCollect; ++Local)
				{
					Consider(Local);
				}
			}
			else
			{
				for (int32 Y = MinY; Y <= MaxY && Viewers.Num() < MaxViewersToCollect; ++Y)
				{
					for (int32 X = MinX; X <= MaxX && Viewers.Num() < MaxViewersToCollect; ++X)
					{
						for (int32 Local = Grid.Head[CellHash(X, Y) & Grid.Mask]; Local != INDEX_NONE && Viewers.Num() < MaxViewersToCollect; Local = Grid.Next[Local])
						{
							if (Grid.CellX[Local] == X && Grid.CellY[Local] == Y)
							{
								Consider(Local);
							}
						}
					}
				}
			}
			if (Viewers.IsEmpty())
			{
				continue;
			}

			bool bVisible = true;
			if (LineOfSight)
			{
				TMap<FMassEntityHandle, FLineOfSightAnswer>& TeamCache = LineOfSightCache.FindOrAdd(Grid.TeamId);
				FLineOfSightAnswer* Cached = TeamCache.Find(Candidate.Entity);
				if (Cached && Now - Cached->Time < LineOfSightCacheSeconds)
				{
					bVisible = Cached->bVisible;
				}
				else if (!LineOfSight->HasBudget())
				{
					bVisible = Cached ? Cached->bVisible : false; // out of raycasts this frame: keep the last answer
				}
				else
				{
					Algo::Sort(Viewers, [](const FViewer& A, const FViewer& B) { return A.DistSq < B.DistSq; });
					bVisible = false;
					for (int32 Test = 0; Test < FMath::Min(TestsPerUnit, Viewers.Num()); ++Test)
					{
						if (LineOfSight->TraceLineOfSight(Units[Viewers[Test].Index].Location, Candidate.Location))
						{
							bVisible = true;
							break;
						}
						if (!LineOfSight->HasBudget())
						{
							break;
						}
					}
					FLineOfSightAnswer& Answer = TeamCache.FindOrAdd(Candidate.Entity);
					Answer.Time = Now;
					Answer.bVisible = bVisible;
				}
			}

			if (bVisible)
			{
				NewVisibility.FindOrAdd(Grid.TeamId).Add(Candidate.Entity);
			}
		}
	}

	// Forget answers about units that have not been asked about for a while (dead, out of range, ...).
	for (TPair<uint8, TMap<FMassEntityHandle, FLineOfSightAnswer>>& TeamPair : LineOfSightCache)
	{
		for (auto It = TeamPair.Value.CreateIterator(); It; ++It)
		{
			if (Now - It->Value.Time > 10.0 * FMath::Max(LineOfSightCacheSeconds, UpdateInterval))
			{
				It.RemoveCurrent();
			}
		}
	}

	VisibilitySubsystem->SetTeamVisibility(MoveTemp(NewVisibility));
}
