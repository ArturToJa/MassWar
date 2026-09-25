// Copyright Epic Games, Inc. All Rights Reserved.

#include "Processors/MassWarSeparationProcessor.h"
#include "Processors/MassWarOrderMovementProcessor.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "MassExecutionContext.h"
#include "Fragments/MassWarUnitFragments.h"
#include "Async/ParallelFor.h"

namespace
{
	constexpr uint8 FlagActive = 1 << 0;
	constexpr uint8 FlagMoving = 1 << 1;

	/** Standing units ignore overlaps smaller than this fraction of the combined radii, and are pushed gently. */
	constexpr float StandingDeadZone = 0.3f;
	constexpr float StandingPushScale = 0.5f;
	/** How much of the push a moving unit turns into a sidestep to its right (breaks head-on symmetry). */
	constexpr float SidestepScale = 0.35f;
	/** A Move order blocked by standing units gives up this many radii (plus its stop distance) from the goal. */
	constexpr float BlockedArrivalRadii = 4.f;
	constexpr float MovingSpeedSquared = 25.f; // (5 uu/s)^2

	FORCEINLINE uint32 CellHash(int32 X, int32 Y)
	{
		return static_cast<uint32>(X) * 73856093u ^ static_cast<uint32>(Y) * 19349663u;
	}
}

UMassWarSeparationProcessor::UMassWarSeparationProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
	// After the steering velocity is written, before the engine's simple movement integrates it.
	ExecutionOrder.ExecuteAfter.Add(UMassWarOrderMovementProcessor::StaticClass()->GetFName());
	ExecutionOrder.ExecuteBefore.Add(TEXT("MassSimpleMovementProcessor"));
}

void UMassWarSeparationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassWarAvoidanceFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassWarMovementParamsFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassWarLifeFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassWarOrderFragment>(EMassFragmentAccess::ReadWrite);
}

void UMassWarSeparationProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	// Pass 1: gather every avoiding unit into flat arrays (chunk order; pass 2 walks the same order).
	Positions.Reset();
	Radii.Reset();
	Flags.Reset();
	float MaxRadius = 0.f;

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TConstArrayView<FMassWarAvoidanceFragment> AvoidanceList = Context.GetFragmentView<FMassWarAvoidanceFragment>();
		const TConstArrayView<FMassWarLifeFragment> LifeList = Context.GetFragmentView<FMassWarLifeFragment>();
		const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FMassVelocityFragment> VelocityList = Context.GetFragmentView<FMassVelocityFragment>();

		for (int32 Index = 0; Index < Context.GetNumEntities(); ++Index)
		{
			const FVector Location = TransformList[Index].GetTransform().GetLocation();
			const float Radius = AvoidanceList[Index].Radius;
			Positions.Add(FVector2f(static_cast<float>(Location.X), static_cast<float>(Location.Y)));
			Radii.Add(Radius);
			uint8 UnitFlags = 0;
			if (!LifeList[Index].IsDying())
			{
				UnitFlags |= FlagActive;
				MaxRadius = FMath::Max(MaxRadius, Radius);
				if (VelocityList[Index].Value.SizeSquared2D() > MovingSpeedSquared)
				{
					UnitFlags |= FlagMoving;
				}
			}
			Flags.Add(UnitFlags);
		}
	});

	const int32 Num = Positions.Num();
	if (Num < 2 || MaxRadius <= 0.f)
	{
		return;
	}

	// Hashed uniform grid: cells are two max-radii wide so overlapping units always sit in adjacent cells.
	// Buckets may merge several cells; the stored cell coordinates keep each neighbour visited exactly once.
	const float InvCellSize = 1.f / (2.f * MaxRadius);
	const uint32 HashSize = FMath::RoundUpToPowerOfTwo(static_cast<uint32>(Num) * 2u + 16u);
	const uint32 HashMask = HashSize - 1u;

	Head.Reset();
	Head.Init(INDEX_NONE, HashSize);
	Next.SetNumUninitialized(Num, EAllowShrinking::No);
	CellX.SetNumUninitialized(Num, EAllowShrinking::No);
	CellY.SetNumUninitialized(Num, EAllowShrinking::No);
	for (int32 Index = 0; Index < Num; ++Index)
	{
		if (!(Flags[Index] & FlagActive))
		{
			continue;
		}
		const int32 X = FMath::FloorToInt(Positions[Index].X * InvCellSize);
		const int32 Y = FMath::FloorToInt(Positions[Index].Y * InvCellSize);
		CellX[Index] = X;
		CellY[Index] = Y;
		const uint32 Bucket = CellHash(X, Y) & HashMask;
		Next[Index] = Head[Bucket];
		Head[Bucket] = Index;
	}

	// Sum a push away from every overlapping neighbour. Read-only on shared data, so it runs in parallel.
	Push.SetNumUninitialized(Num, EAllowShrinking::No);
	Blocked.SetNumUninitialized(Num, EAllowShrinking::No);
	ParallelFor(Num, [this, InvCellSize, HashMask](int32 Index)
	{
		FVector2f Sum(0.f, 0.f);
		uint8 bBlocked = 0;
		if (Flags[Index] & FlagActive)
		{
			const FVector2f Position = Positions[Index];
			const float Radius = Radii[Index];
			const int32 CenterX = FMath::FloorToInt(Position.X * InvCellSize);
			const int32 CenterY = FMath::FloorToInt(Position.Y * InvCellSize);

			for (int32 DY = -1; DY <= 1; ++DY)
			{
				for (int32 DX = -1; DX <= 1; ++DX)
				{
					const int32 X = CenterX + DX;
					const int32 Y = CenterY + DY;
					for (int32 Other = Head[CellHash(X, Y) & HashMask]; Other != INDEX_NONE; Other = Next[Other])
					{
						if (Other == Index || CellX[Other] != X || CellY[Other] != Y)
						{
							continue;
						}
						const FVector2f Delta = Position - Positions[Other];
						const float Combined = Radius + Radii[Other];
						const float DistSquared = Delta.SizeSquared();
						if (DistSquared >= Combined * Combined)
						{
							continue;
						}
						const float Dist = FMath::Sqrt(DistSquared);
						FVector2f Direction;
						if (Dist > 0.001f)
						{
							Direction = Delta / Dist;
						}
						else
						{
							// Exactly stacked (e.g. spawned on one point): split along a per-unit direction.
							const float Angle = static_cast<float>(Index) * 2.39996f;
							Direction = FVector2f(FMath::Cos(Angle), FMath::Sin(Angle));
						}
						Sum += Direction * ((Combined - Dist) / Combined);
						if (!(Flags[Other] & FlagMoving))
						{
							bBlocked = 1;
						}
					}
				}
			}

			if (Flags[Index] & FlagMoving)
			{
				// Sidestep to the right of the push so head-on pairs slide past each other.
				Sum += FVector2f(-Sum.Y, Sum.X) * SidestepScale;
			}
			else if (Sum.Size() < StandingDeadZone)
			{
				Sum = FVector2f(0.f, 0.f);
			}
			else
			{
				Sum *= StandingPushScale;
			}
		}
		Push[Index] = Sum;
		Blocked[Index] = bBlocked;
	}, EParallelForFlags::None);

	// Pass 2: add the push to the steering velocity (same chunk order as pass 1).
	int32 GlobalIndex = 0;
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TConstArrayView<FMassWarAvoidanceFragment> AvoidanceList = Context.GetFragmentView<FMassWarAvoidanceFragment>();
		const TConstArrayView<FMassWarMovementParamsFragment> ParamsList = Context.GetFragmentView<FMassWarMovementParamsFragment>();
		const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
		const TArrayView<FMassVelocityFragment> VelocityList = Context.GetMutableFragmentView<FMassVelocityFragment>();
		const TArrayView<FMassWarOrderFragment> OrderList = Context.GetMutableFragmentView<FMassWarOrderFragment>();

		for (int32 Index = 0; Index < Context.GetNumEntities(); ++Index, ++GlobalIndex)
		{
			if (!(Flags[GlobalIndex] & FlagActive))
			{
				continue;
			}
			const FMassWarMovementParamsFragment& Params = ParamsList[Index];
			FMassWarOrderFragment& Order = OrderList[Index];

			// A Move order held up by standing units close to its goal is as arrived as it will get.
			if (Blocked[GlobalIndex] && Order.OrderType == EMassWarOrderType::Move)
			{
				const float StopDistance = Order.bStopAtAttackDistance ? Params.AttackStopDistance : Params.AcceptanceRadius;
				const FVector Location = TransformList[Index].GetTransform().GetLocation();
				if (FVector::Dist2D(Location, Order.Destination) <= StopDistance + BlockedArrivalRadii * AvoidanceList[Index].Radius)
				{
					Order.OrderType = EMassWarOrderType::Idle;
					Order.bPlayerCommanded = false;
					VelocityList[Index].Value = FVector::ZeroVector;
				}
			}

			const FVector2f UnitPush = Push[GlobalIndex];
			if (UnitPush.IsNearlyZero())
			{
				continue;
			}
			FVector2f Add = UnitPush * (Params.MoveSpeed * AvoidanceList[Index].Strength);
			const float MaxAdd = Params.MoveSpeed;
			const float AddSize = Add.Size();
			if (AddSize > MaxAdd)
			{
				Add *= MaxAdd / AddSize;
			}
			VelocityList[Index].Value += FVector(Add.X, Add.Y, 0.0);
		}
	});
}
