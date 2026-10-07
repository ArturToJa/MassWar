// Copyright Epic Games, Inc. All Rights Reserved.

#include "Processors/MassWarPerceptionProcessor.h"
#include "Fragments/MassWarPerceptionFragments.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "Fragments/MassWarUnitFragments.h"
#include "UnitBrain/MassWarUnitStateView.h"
#include "Sight/MassWarLineOfSightSubsystem.h"
#include "Engine/World.h"

namespace
{
	FORCEINLINE uint32 CellHash(int32 X, int32 Y)
	{
		return static_cast<uint32>(X) * 73856093u ^ static_cast<uint32>(Y) * 19349663u;
	}

	/** Noise ranges are resolved out to this multiple of their range, the most a hearing sensitivity can reach. */
	constexpr float MaxHearingSensitivity = 2.f;

	/** Sight considers this many nearest candidates so that, with line of sight on, units hidden behind cover
	 *  do not use up the slots of visible ones farther away. */
	constexpr int32 MaxCandidates = MassWarPerceptionMaxTracked * 2;
}

UMassWarPerceptionProcessor::UMassWarPerceptionProcessor()
	: TargetQuery(*this)
	, ObserverQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
	// Combat reports damage and noise while it runs; perception resolves them afterwards (and, being ordered,
	// never touches a unit's perception data at the same time as combat does).
	ExecutionOrder.ExecuteAfter.Add(TEXT("MassWarDamageProcessor"));
	// Line-of-sight raycasts are game-thread-only.
	bRequiresGameThreadExecution = true;
}

void UMassWarPerceptionProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	TargetQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	TargetQuery.AddRequirement<FMassWarTeamFragment>(EMassFragmentAccess::ReadOnly);
	TargetQuery.AddRequirement<FMassWarLifeFragment>(EMassFragmentAccess::ReadOnly);

	ObserverQuery.AddRequirement<FMassWarPerceptionFragment>(EMassFragmentAccess::ReadWrite);
	ObserverQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	ObserverQuery.AddRequirement<FMassWarTeamFragment>(EMassFragmentAccess::ReadOnly);
	ObserverQuery.AddRequirement<FMassWarLifeFragment>(EMassFragmentAccess::ReadOnly);
	ObserverQuery.AddConstSharedRequirement<FMassWarPerceptionParams>(EMassFragmentPresence::All);
}

void UMassWarPerceptionProcessor::RebuildGrid(FMassExecutionContext& Context)
{
	GridEntities.Reset();
	GridPositions.Reset();
	GridTeams.Reset();

	TargetQuery.ForEachEntityChunk(Context, [this](FMassExecutionContext& Context)
	{
		const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FMassWarTeamFragment> TeamList = Context.GetFragmentView<FMassWarTeamFragment>();
		const TConstArrayView<FMassWarLifeFragment> LifeList = Context.GetFragmentView<FMassWarLifeFragment>();

		for (int32 Index = 0; Index < Context.GetNumEntities(); ++Index)
		{
			if (LifeList[Index].IsDying())
			{
				continue;
			}
			GridEntities.Add(Context.GetEntity(Index));
			GridPositions.Add(FVector3f(TransformList[Index].GetTransform().GetLocation()));
			GridTeams.Add(TeamList[Index].TeamId);
		}
	});

	const int32 Num = GridEntities.Num();
	InvCellSize = 1.f / GridCellSize;
	const uint32 HashSize = FMath::RoundUpToPowerOfTwo(static_cast<uint32>(Num) * 2u + 16u);
	HashMask = HashSize - 1u;

	GridHead.Reset();
	GridHead.Init(INDEX_NONE, HashSize);
	GridNext.SetNumUninitialized(Num, EAllowShrinking::No);
	GridCellX.SetNumUninitialized(Num, EAllowShrinking::No);
	GridCellY.SetNumUninitialized(Num, EAllowShrinking::No);
	for (int32 Index = 0; Index < Num; ++Index)
	{
		const int32 X = FMath::FloorToInt(GridPositions[Index].X * InvCellSize);
		const int32 Y = FMath::FloorToInt(GridPositions[Index].Y * InvCellSize);
		GridCellX[Index] = X;
		GridCellY[Index] = Y;
		const uint32 Bucket = CellHash(X, Y) & HashMask;
		GridNext[Index] = GridHead[Bucket];
		GridHead[Bucket] = Index;
	}
}

template<typename FuncType>
void UMassWarPerceptionProcessor::ForEachInRadius(const FVector3f& Center, float Radius, FuncType&& Func) const
{
	const int32 Num = GridEntities.Num();
	if (Num == 0)
	{
		return;
	}
	const float RadiusSq = Radius * Radius;
	const int32 MinX = FMath::FloorToInt((Center.X - Radius) * InvCellSize);
	const int32 MaxX = FMath::FloorToInt((Center.X + Radius) * InvCellSize);
	const int32 MinY = FMath::FloorToInt((Center.Y - Radius) * InvCellSize);
	const int32 MaxY = FMath::FloorToInt((Center.Y + Radius) * InvCellSize);

	// A radius that would visit more cells than there are units (huge radius, few units): just scan the units.
	if (static_cast<int64>(MaxX - MinX + 1) * (MaxY - MinY + 1) > Num)
	{
		for (int32 Index = 0; Index < Num; ++Index)
		{
			if (FVector2f::DistSquared(FVector2f(Center), FVector2f(GridPositions[Index])) <= RadiusSq)
			{
				Func(Index);
			}
		}
		return;
	}

	for (int32 Y = MinY; Y <= MaxY; ++Y)
	{
		for (int32 X = MinX; X <= MaxX; ++X)
		{
			for (int32 Index = GridHead[CellHash(X, Y) & HashMask]; Index != INDEX_NONE; Index = GridNext[Index])
			{
				if (GridCellX[Index] == X && GridCellY[Index] == Y
					&& FVector2f::DistSquared(FVector2f(Center), FVector2f(GridPositions[Index])) <= RadiusSq)
				{
					Func(Index);
				}
			}
		}
	}
}

void UMassWarPerceptionProcessor::ProcessNoises(FMassEntityManager& EntityManager, double Now)
{
	for (const FMassWarNoiseEvent& Noise : Noises)
	{
		const FVector3f NoiseLocation(Noise.Location);
		ForEachInRadius(NoiseLocation, Noise.Range * MaxHearingSensitivity, [&](int32 GridIndex)
		{
			const FMassEntityHandle Hearer = GridEntities[GridIndex];
			if (Hearer == Noise.Instigator
				|| MassWarGetAffiliation(Noise.InstigatorTeam, GridTeams[GridIndex]) != EMassWarAffiliation::Enemy
				|| !EntityManager.IsEntityValid(Hearer))
			{
				return;
			}
			FMassWarPerceptionFragment* Perception = EntityManager.GetFragmentDataPtr<FMassWarPerceptionFragment>(Hearer);
			const FMassWarPerceptionParams* Params = EntityManager.GetConstSharedFragmentDataPtr<FMassWarPerceptionParams>(Hearer);
			if (!Perception || !Params || !Params->bCanHear)
			{
				return;
			}
			const float HearingRange = Noise.Range * Params->HearingSensitivity;
			const float Distance = FVector2f::Distance(FVector2f(NoiseLocation), FVector2f(GridPositions[GridIndex]));
			if (Distance > HearingRange)
			{
				return;
			}
			FMassWarPerceivedEntry* Entry = Perception->FindOrAdd(Noise.Instigator);
			if (!Entry)
			{
				return;
			}
			if (!Entry->bSeenNow)
			{
				Entry->LastKnownLocation = NoiseLocation; // sight already knows better where a seen unit is
			}
			Entry->LastStimulusTime = Now;
			Entry->LastHeardTime = Now;
			Entry->LastSense = EMassWarSense::Hearing;
			Entry->Strength = Noise.Loudness * (1.f - Distance / HearingRange);
			Entry->Tag = Noise.Tag;
		});
	}
}

void UMassWarPerceptionProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* World = GetWorld();
	UMassWarPerceptionSubsystem* Subsystem = World ? World->GetSubsystem<UMassWarPerceptionSubsystem>() : nullptr;
	if (!Subsystem)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();

	if (Now >= NextGridBuildTime)
	{
		RebuildGrid(Context);
		NextGridBuildTime = Now + GridRebuildInterval;
	}

	Subsystem->ConsumeNoises(Noises);
	ProcessNoises(EntityManager, Now);

	UMassWarLineOfSightSubsystem* LineOfSight = World->GetSubsystem<UMassWarLineOfSightSubsystem>();

	ObserverQuery.ForEachEntityChunk(Context, [this, &EntityManager, Now, LineOfSight](FMassExecutionContext& Context)
	{
		const FMassWarPerceptionParams& Params = Context.GetConstSharedFragment<FMassWarPerceptionParams>();
		const TArrayView<FMassWarPerceptionFragment> PerceptionList = Context.GetMutableFragmentView<FMassWarPerceptionFragment>();
		const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FMassWarTeamFragment> TeamList = Context.GetFragmentView<FMassWarTeamFragment>();
		const TConstArrayView<FMassWarLifeFragment> LifeList = Context.GetFragmentView<FMassWarLifeFragment>();

		const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(Params.SightHalfAngleDegrees));

		for (int32 Index = 0; Index < Context.GetNumEntities(); ++Index)
		{
			FMassWarPerceptionFragment& Perception = PerceptionList[Index];

			if (LifeList[Index].IsDying())
			{
				Perception.Num = 0;
				continue;
			}

			// Spread units' scans over the interval instead of scanning all of them in the same frame.
			const FMassEntityHandle Self = Context.GetEntity(Index);
			if (Perception.NextUpdateTime == 0.0)
			{
				const uint32 Jitter = (GetTypeHash(Self) * 2654435761u) >> 16;
				Perception.NextUpdateTime = Now + Params.SightUpdateInterval * (static_cast<double>(Jitter & 0xFFFF) / 65536.0);
			}
			if (Now < Perception.NextUpdateTime)
			{
				continue;
			}
			Perception.NextUpdateTime = Now + Params.SightUpdateInterval;

			// Forget the dead and the long-unseen.
			for (int32 EntryIndex = Perception.Num - 1; EntryIndex >= 0; --EntryIndex)
			{
				const FMassWarPerceivedEntry& Entry = Perception.Entries[EntryIndex];
				if (!FMassWarUnitStateView::IsLiving(EntityManager, Entry.Entity)
					|| (!Entry.bSeenNow && Entry.GetAge(Now) > Params.MemoryDuration))
				{
					Perception.RemoveAt(EntryIndex);
				}
			}

			// Sight: gather the nearest visible enemies (using last update's seen-state for the lose-sight margin)...
			struct FCandidate { int32 GridIndex; float DistSq; };
			FCandidate Candidates[MaxCandidates];
			int32 NumCandidates = 0;

			if (Params.bCanSee)
			{
				const FTransform& Transform = TransformList[Index].GetTransform();
				const FVector3f Position(Transform.GetLocation());
				const FVector ForwardVector = Transform.GetRotation().GetForwardVector();
				FVector2f Forward(static_cast<float>(ForwardVector.X), static_cast<float>(ForwardVector.Y));
				Forward.Normalize();
				const uint8 SelfTeam = TeamList[Index].TeamId;
				const float SightSq = Params.SightRadius * Params.SightRadius;

				ForEachInRadius(Position, Params.LoseSightRadius, [&](int32 GridIndex)
				{
					const FMassEntityHandle Other = GridEntities[GridIndex];
					if (Other == Self || MassWarGetAffiliation(SelfTeam, GridTeams[GridIndex]) != EMassWarAffiliation::Enemy)
					{
						return;
					}
					const FVector2f ToOther = FVector2f(GridPositions[GridIndex]) - FVector2f(Position);
					const float DistSq = ToOther.SizeSquared();
					if (DistSq > SightSq)
					{
						const FMassWarPerceivedEntry* Known = Perception.Find(Other);
						if (!Known || !Known->bSeenNow)
						{
							return; // beyond sight radius, and not yet seen: only already-seen units survive out to the lose radius
						}
					}
					if (DistSq > 1.f && FVector2f::DotProduct(Forward, ToOther) < CosHalfAngle * FMath::Sqrt(DistSq))
					{
						return; // outside the vision cone
					}

					// Insert into the (sorted, nearest-first) candidate list, dropping the farthest if it is full.
					int32 Slot = NumCandidates;
					while (Slot > 0 && Candidates[Slot - 1].DistSq > DistSq)
					{
						--Slot;
					}
					if (Slot >= MaxCandidates)
					{
						return;
					}
					const int32 Last = FMath::Min(NumCandidates, MaxCandidates - 1);
					for (int32 Move = Last; Move > Slot; --Move)
					{
						Candidates[Move] = Candidates[Move - 1];
					}
					Candidates[Slot] = { GridIndex, DistSq };
					NumCandidates = FMath::Min(NumCandidates + 1, MaxCandidates);
				});
			}

			// ...then everything not in that set is no longer seen, and the set is committed.
			for (int32 EntryIndex = 0; EntryIndex < Perception.Num; ++EntryIndex)
			{
				Perception.Entries[EntryIndex].bSeenNow = false;
			}
			int32 NumAccepted = 0;
				for (int32 CandidateIndex = 0; CandidateIndex < NumCandidates && NumAccepted < MassWarPerceptionMaxTracked; ++CandidateIndex)
			{
				const FCandidate& Candidate = Candidates[CandidateIndex];
				const FMassEntityHandle CandidateEntity = GridEntities[Candidate.GridIndex];

					// Line of sight: level geometry hides a unit even inside the vision cone. The answer is remembered on the
					// entry for LineOfSightRecheckInterval; when the shared raycast budget is spent the last answer stands.
					FMassWarPerceivedEntry* Known = Perception.FindMutable(CandidateEntity);
					if (LineOfSight && Params.bRequireLineOfSight)
					{
						bool bHasLineOfSight;
						if (Known && Now - Known->LastLineOfSightCheck < Params.LineOfSightRecheckInterval)
						{
							bHasLineOfSight = Known->bHadLineOfSight;
						}
						else if (LineOfSight->HasBudget())
						{
							bHasLineOfSight = LineOfSight->TraceLineOfSight(FVector(TransformList[Index].GetTransform().GetLocation()), FVector(GridPositions[Candidate.GridIndex]));
						}
						else
						{
							bHasLineOfSight = Known ? Known->bHadLineOfSight : false;
						}
						if (Known)
						{
							Known->LastLineOfSightCheck = Now;
							Known->bHadLineOfSight = bHasLineOfSight;
						}
						if (!bHasLineOfSight)
						{
							continue; // hidden: stays (at most) a memory; nothing is added for a unit that was never seen
						}
					}

					FMassWarPerceivedEntry* Entry = Perception.FindOrAdd(CandidateEntity);
				if (!Entry)
				{
					continue;
				}
				++NumAccepted;
					Entry->bHadLineOfSight = true;
					Entry->LastLineOfSightCheck = Known ? Entry->LastLineOfSightCheck : Now;
					Entry->LastKnownLocation = GridPositions[Candidate.GridIndex];
				Entry->LastStimulusTime = Now;
				Entry->LastSense = EMassWarSense::Sight;
				Entry->bSeenNow = true;
				Entry->Strength = 1.f - FMath::Sqrt(Candidate.DistSq) / FMath::Max(Params.SightRadius, 1.f);
			}
		}
	});
}
