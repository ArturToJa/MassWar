// Copyright Epic Games, Inc. All Rights Reserved.

#include "Processors/MassWarOrderMovementProcessor.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "MassExecutionContext.h"
#include "Fragments/MassWarUnitFragments.h"

UMassWarOrderMovementProcessor::UMassWarOrderMovementProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
}

void UMassWarOrderMovementProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassWarOrderFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassWarMovementParamsFragment>(EMassFragmentAccess::ReadOnly);
}

void UMassWarOrderMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, [&EntityManager](FMassExecutionContext& Context)
	{
		const TArrayView<FMassWarOrderFragment> Orders = Context.GetMutableFragmentView<FMassWarOrderFragment>();
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();
		const TArrayView<FMassVelocityFragment> Velocities = Context.GetMutableFragmentView<FMassVelocityFragment>();
		const TConstArrayView<FMassWarMovementParamsFragment> MovementParamsList = Context.GetFragmentView<FMassWarMovementParamsFragment>();

		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			FMassWarOrderFragment& Order = Orders[It];

			// Move orders head for their destination; Attack orders (which, once bPlayerCommanded blocks
			// StateTree's own auto-engage chase, can no longer rely on AI movement) close the distance to
			// their live target instead - MassWarCombat's damage processor only cares about being in range,
			// it doesn't move anyone. Anything else (Idle) has nowhere to go.
			FVector Destination;
			if (Order.OrderType == EMassWarOrderType::Move)
			{
				Destination = Order.Destination;
			}
			else if (Order.OrderType == EMassWarOrderType::Attack && EntityManager.IsEntityValid(Order.TargetEntity))
			{
				const FTransformFragment* TargetTransform = EntityManager.GetFragmentDataPtr<FTransformFragment>(Order.TargetEntity);
				if (!TargetTransform)
				{
					Velocities[It].Value = FVector::ZeroVector;
					continue;
				}
				Destination = TargetTransform->GetTransform().GetLocation();
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

			if (Distance <= MovementParams.AcceptanceRadius)
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
				Velocities[It].Value = (ToDestination / Distance) * MovementParams.MoveSpeed;
			}
		}
	});
}
