// Copyright Epic Games, Inc. All Rights Reserved.

#include "Processors/MassWarNavPathProcessor.h"
#include "Processors/MassWarOrderMovementProcessor.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "Fragments/MassWarUnitFragments.h"
#include "UnitBrain/MassWarUnitStateView.h"
#include "NavigationSystem.h"
#include "NavigationData.h"
#include "Engine/World.h"

UMassWarNavPathProcessor::UMassWarNavPathProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
	// Navigation queries are game-thread API.
	bRequiresGameThreadExecution = true;
	// Paths must be fresh before the movement processor steers along them.
	ExecutionOrder.ExecuteBefore.Add(UMassWarOrderMovementProcessor::StaticClass()->GetFName());
}

void UMassWarNavPathProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassWarNavPathFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassWarOrderFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassWarLifeFragment>(EMassFragmentAccess::ReadOnly);
}

void UMassWarNavPathProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* World = GetWorld();
	UNavigationSystemV1* NavSys = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
	const ANavigationData* NavData = NavSys ? NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) : nullptr;

	const float DeltaTime = Context.GetDeltaTimeSeconds();
	int32 RequestsLeft = MaxPathRequestsPerFrame;

	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& Context)
	{
		const TArrayView<FMassWarNavPathFragment> NavList = Context.GetMutableFragmentView<FMassWarNavPathFragment>();
		const TConstArrayView<FMassWarOrderFragment> OrderList = Context.GetFragmentView<FMassWarOrderFragment>();
		const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FMassWarLifeFragment> LifeList = Context.GetFragmentView<FMassWarLifeFragment>();

		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			FMassWarNavPathFragment& Nav = NavList[It];
			const FMassWarOrderFragment& Order = OrderList[It];
			Nav.TimeSinceRequest += DeltaTime;

			// What is this unit heading for? Only Move and Attack orders go anywhere.
			FVector Goal = FVector::ZeroVector;
			FMassEntityHandle GoalEntity;
			if (LifeList[It].IsDying())
			{
				Nav.State = EMassWarNavPathState::None;
				continue;
			}
			else if (Order.OrderType == EMassWarOrderType::Move)
			{
				Goal = Order.Destination;
			}
			else if (Order.OrderType == EMassWarOrderType::Attack)
			{
				const FMassWarUnitStateView TargetView = FMassWarUnitStateView::FromHandle(EntityManager, FMassWarUnitHandle(Order.TargetEntity));
				if (!TargetView.IsValid())
				{
					continue; // combat is about to drop this order
				}
				Goal = TargetView.GetLocation();
				GoalEntity = Order.TargetEntity;
			}
			else
			{
				// Idle: nothing to route. Forget the old path so the next order starts clean.
				if (Nav.State != EMassWarNavPathState::None)
				{
					Nav.Waypoints.Reset();
					Nav.NextWaypoint = 0;
					Nav.State = EMassWarNavPathState::None;
					Nav.PathTargetEntity.Reset();
				}
				continue;
			}

			// Does the order need a (new) path?
			const float GoalMoved = FVector::Dist2D(Goal, Nav.PathDestination);
			bool bNeedsPath;
			if (Nav.State == EMassWarNavPathState::None || GoalEntity != Nav.PathTargetEntity)
			{
				bNeedsPath = true; // a new order
			}
			else if (!GoalEntity.IsValid() && GoalMoved > 50.f)
			{
				bNeedsPath = true; // a Move order with a new destination
			}
			else if (GoalEntity.IsValid())
			{
				// Chasing a moving target: repath only when it has moved far enough, and not too often.
				bNeedsPath = GoalMoved > Nav.RepathDistance && Nav.TimeSinceRequest >= Nav.RepathInterval;
			}
			else
			{
				bNeedsPath = false;
			}
			if (!bNeedsPath && Nav.TimeSinceRequest >= MaxPathAgeSeconds)
			{
				bNeedsPath = true;
			}
			if (!bNeedsPath)
			{
				continue;
			}

			if (RequestsLeft <= 0)
			{
				continue; // over budget this frame - try again next frame
			}
			--RequestsLeft;

			Nav.Waypoints.Reset();
			Nav.NextWaypoint = 0;
			Nav.PathDestination = Goal;
			Nav.PathTargetEntity = GoalEntity;
			Nav.TimeSinceRequest = 0.f;

			if (!NavData)
			{
				Nav.State = EMassWarNavPathState::Failed; // no navmesh in this level: straight-line movement
				continue;
			}

			const FVector Start = TransformList[It].GetTransform().GetLocation();

			// Cheap first: if nothing on the navmesh blocks the straight line, the path is just the goal.
			FVector HitLocation;
			if (!UNavigationSystemV1::NavigationRaycast(World, Start, Goal, HitLocation))
			{
				Nav.Waypoints.Add(Goal);
				Nav.State = EMassWarNavPathState::Ready;
				continue;
			}

			// Blocked: pay for a real path. The goal need not be on the navmesh - a partial path to the nearest
			// reachable point is fine (a unit ordered onto an obstacle gets as close as it can).
			const FPathFindingQuery Query((const UObject*)nullptr, *NavData, Start, Goal, nullptr, nullptr,
				TNumericLimits<FVector::FReal>::Max(), /*bRequireNavigableEndLocation=*/ false);
			const FPathFindingResult Result = NavSys->FindPathSync(Query);
			if (Result.IsSuccessful() && Result.Path.IsValid() && Result.Path->GetPathPoints().Num() > 1)
			{
				const TArray<FNavPathPoint>& Points = Result.Path->GetPathPoints();
				Nav.Waypoints.Reserve(Points.Num() - 1);
				for (int32 PointIndex = 1; PointIndex < Points.Num(); ++PointIndex) // [0] is the start
				{
					Nav.Waypoints.Add(Points[PointIndex].Location);
				}
				Nav.State = EMassWarNavPathState::Ready;
			}
			else
			{
				Nav.State = EMassWarNavPathState::Failed;
			}
		}
	});
}
