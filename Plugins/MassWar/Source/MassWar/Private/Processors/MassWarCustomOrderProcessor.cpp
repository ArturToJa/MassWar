// Copyright Epic Games, Inc. All Rights Reserved.

#include "Processors/MassWarCustomOrderProcessor.h"
#include "Processors/MassWarOrderMovementProcessor.h"
#include "Orders/MassWarOrderSubsystem.h"
#include "Orders/MassWarOrderBehavior.h"
#include "Orders/MassWarOrderDefinition.h"
#include "Fragments/MassWarUnitFragments.h"
#include "MassExecutionContext.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassWarCustomOrderProcessor)

UMassWarCustomOrderProcessor::UMassWarCustomOrderProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
	bRequiresGameThreadExecution = true;
	// Before units are moved, so a Move handed over this frame already takes effect.
	ExecutionOrder.ExecuteBefore.Add(UMassWarOrderMovementProcessor::StaticClass()->GetFName());
}

void UMassWarCustomOrderProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassWarCustomOrderFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassWarOrderFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassWarLifeFragment>(EMassFragmentAccess::ReadOnly);
}

void UMassWarCustomOrderProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* World = EntityManager.GetWorld();
	UMassWarOrderSubsystem* Orders = World ? World->GetSubsystem<UMassWarOrderSubsystem>() : nullptr;
	if (!Orders)
	{
		return;
	}
	const float Now = static_cast<float>(World->GetTimeSeconds());

	EntityQuery.ForEachEntityChunk(Context, [&EntityManager, Orders, Now](FMassExecutionContext& Context)
	{
		const TArrayView<FMassWarCustomOrderFragment> CustomList = Context.GetMutableFragmentView<FMassWarCustomOrderFragment>();
		const TConstArrayView<FMassWarOrderFragment> PrimitiveList = Context.GetFragmentView<FMassWarOrderFragment>();
		const TConstArrayView<FMassWarLifeFragment> LifeList = Context.GetFragmentView<FMassWarLifeFragment>();

		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			FMassWarCustomOrderFragment& Custom = CustomList[It];
			if (!Custom.IsActive())
			{
				continue;
			}

			const FMassEntityHandle Entity = Context.GetEntity(It);
			if (LifeList[It].IsDying())
			{
				Orders->CancelOrder(EntityManager, Entity, /*bStopUnit=*/ false);
				continue;
			}

			const UMassWarOrderBehavior* Behavior = Orders->GetBehavior(Custom.OrderId);
			if (!Behavior)
			{
				Custom.Reset();
				continue;
			}

			// The Move / Attack the order handed over has ended (the unit is idle again).
			if (Custom.bAwaitingPrimitive && PrimitiveList[It].OrderType == EMassWarOrderType::Idle)
			{
				Custom.bAwaitingPrimitive = false;
				Behavior->OnPrimitiveFinished(Orders->MakeContext(EntityManager, Entity, Custom));
				if (!Custom.IsActive())
				{
					continue; // the behaviour finished the order
				}
			}

			if (Now >= Custom.NextTickTime)
			{
				const UMassWarOrderDefinition* Definition = Orders->GetDefinition(Custom.OrderId);
				Custom.NextTickTime = Now + (Definition ? Definition->TickInterval : 0.25f);
				Behavior->OnTick(Orders->MakeContext(EntityManager, Entity, Custom));
			}
		}
	});
}