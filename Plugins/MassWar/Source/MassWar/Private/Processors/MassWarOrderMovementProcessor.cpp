// Copyright Epic Games, Inc. All Rights Reserved.

#include "Processors/MassWarOrderMovementProcessor.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "MassExecutionContext.h"
#include "Fragments/MassWarUnitFragments.h"
#include "UnitBrain/MassWarUnitStateView.h"

namespace
{
	/** A unit counts as having reached a path corner within this distance and turns toward the next one. */
	constexpr float WaypointReachedRadius = 80.f;
}

UMassWarOrderMovementProcessor::UMassWarOrderMovementProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
}

void UMassWarOrderMovementProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassWarOrderFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassWarMovementParamsFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassWarLifeFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassWarNavPathFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::Optional);
}

void UMassWarOrderMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const float DeltaTime = Context.GetDeltaTimeSeconds();

	EntityQuery.ForEachEntityChunk(Context, [&EntityManager, DeltaTime](FMassExecutionContext& Context)
	{
		const TArrayView<FMassWarOrderFragment> Orders = Context.GetMutableFragmentView<FMassWarOrderFragment>();
		const TArrayView<FTransformFragment> Transforms = Context.GetMutableFragmentView<FTransformFragment>();
		const TArrayView<FMassVelocityFragment> Velocities = Context.GetMutableFragmentView<FMassVelocityFragment>();
		const TConstArrayView<FMassWarMovementParamsFragment> MovementParamsList = Context.GetFragmentView<FMassWarMovementParamsFragment>();
		const TConstArrayView<FMassWarLifeFragment> LifeList = Context.GetFragmentView<FMassWarLifeFragment>();
		// Only present for units that route around obstacles (bUseNavMesh); empty for the rest.
		const TArrayView<FMassWarNavPathFragment> NavList = Context.GetMutableFragmentView<FMassWarNavPathFragment>();

		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			FMassWarOrderFragment& Order = Orders[It];

			// A dying unit stands where it fell (its visual plays the death animation there).
			if (LifeList[It].IsDying())
			{
				Velocities[It].Value = FVector::ZeroVector;
				continue;
			}

			// Move orders head for their destination; Attack orders (which, once bPlayerCommanded blocks
			// StateTree's own auto-engage chase, can no longer rely on AI movement) close the distance to
			// their live target instead - MassWarCombat's damage processor only cares about being in range,
			// it doesn't move anyone. Anything else (Idle) has nowhere to go.
			FVector Destination;
			if (Order.OrderType == EMassWarOrderType::Move)
			{
				Destination = Order.Destination;
			}
			else if (Order.OrderType == EMassWarOrderType::Attack)
			{
				// An invalid view means the target is gone or dying; MassWarCombat's damage processor is
				// what drops the order, movement just stops chasing.
				const FMassWarUnitStateView TargetView = FMassWarUnitStateView::FromHandle(EntityManager, FMassWarUnitHandle(Order.TargetEntity));
				if (!TargetView.IsValid())
				{
					Velocities[It].Value = FVector::ZeroVector;
					continue;
				}
				Destination = TargetView.GetLocation();
			}
			else
			{
				Velocities[It].Value = FVector::ZeroVector;
				continue;
			}

			const FVector CurrentLocation = Transforms[It].GetTransform().GetLocation();
			FVector ToDestination = Destination - CurrentLocation;
			ToDestination.Z = 0.f;

			const float Distance = ToDestination.Size();
			const FMassWarMovementParamsFragment& MovementParams = MovementParamsList[It];

			// Face where we're heading (Move) or the target we're fighting (Attack - including once in range
			// and standing still). Written onto the entity transform so every representation of the unit
			// (near Actor puppet, far ISM instance, replicated client copy - yaw is replicated) agrees.
			const bool bAttacking = Order.OrderType == EMassWarOrderType::Attack;

			// A Move order is done when the unit is really at its destination (AcceptanceRadius); an Attack
			// order just closes in on its target until AttackStopDistance. Two different distances - using
			// one for both made a big attack stand-off distance cut every Move order short.
			const float StopDistance = (bAttacking || Order.bStopAtAttackDistance) ? MovementParams.AttackStopDistance : MovementParams.AcceptanceRadius;

			// Where to steer this frame: straight at the destination - or, for a unit that routes around obstacles
			// (has a navmesh path) and whose path is for THIS order, at the next corner of that path. Arriving and
			// stopping are still judged against the real destination above, so the stop distances mean the same
			// thing with or without navigation.
			FVector SteerTarget = Destination;
			if (!NavList.IsEmpty())
			{
				FMassWarNavPathFragment& Nav = NavList[It];
				const bool bPathIsForThisOrder = Nav.State == EMassWarNavPathState::Ready
					&& (bAttacking
						? Nav.PathTargetEntity == Order.TargetEntity
						: (!Nav.PathTargetEntity.IsValid() && FVector::Dist2D(Nav.PathDestination, Destination) < 100.f));
				if (bPathIsForThisOrder)
				{
					while (Nav.Waypoints.IsValidIndex(Nav.NextWaypoint)
						&& FVector::Dist2D(CurrentLocation, Nav.Waypoints[Nav.NextWaypoint]) <= WaypointReachedRadius)
					{
						++Nav.NextWaypoint;
					}
					if (Nav.Waypoints.IsValidIndex(Nav.NextWaypoint))
					{
						SteerTarget = Nav.Waypoints[Nav.NextWaypoint];
					}
				}
			}
			FVector ToSteer = SteerTarget - CurrentLocation;
			ToSteer.Z = 0.f;
			const float SteerDistance = ToSteer.Size();

			// Face the way we are actually walking; once in range of an attack target, face the target itself.
			const FVector& FaceDirection = (Distance > StopDistance) ? ToSteer : ToDestination;
			const bool bShouldFace = FaceDirection.SizeSquared2D() > 1.f && (bAttacking || Distance > StopDistance);
			if (bShouldFace)
			{
				FTransform& Transform = Transforms[It].GetMutableTransform();
				const float TargetYaw = FMath::RadiansToDegrees(FMath::Atan2(FaceDirection.Y, FaceDirection.X));
				const float NewYaw = FMath::FixedTurn(Transform.GetRotation().Rotator().Yaw, TargetYaw, MovementParams.TurnRate * DeltaTime);
				Transform.SetRotation(FQuat(FVector::UpVector, FMath::DegreesToRadians(NewYaw)));
			}

			if (Distance <= StopDistance)
			{
				Velocities[It].Value = FVector::ZeroVector;
				if (Order.OrderType == EMassWarOrderType::Move)
				{
					Order.OrderType = EMassWarOrderType::Idle;
					Order.bPlayerCommanded = false;
				}
				// Attack orders stay Attack once in range - MassWarCombat's damage processor takes it from here.
			}
			else
			{
				Velocities[It].Value = SteerDistance > UE_SMALL_NUMBER ? (ToSteer / SteerDistance) * MovementParams.MoveSpeed : FVector::ZeroVector;
			}
		}
	});
}
