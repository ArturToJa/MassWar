// Copyright Epic Games, Inc. All Rights Reserved.

#include "Orders/MassWarOrderSubsystem.h"
#include "Orders/MassWarOrderDefinition.h"
#include "Orders/MassWarOrderBehavior.h"
#include "Fragments/MassWarUnitFragments.h"
#include "UnitBrain/MassWarUnitStateView.h"
#include "MassEntityManager.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassWarOrderSubsystem)

void UMassWarOrderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	TArray<const UMassWarOrderDefinition*> Loaded;
	if (const UMassWarOrderSettings* Settings = GetDefault<UMassWarOrderSettings>())
	{
		for (const TSoftObjectPtr<UMassWarOrderDefinition>& Entry : Settings->Orders)
		{
			// A missing asset still takes its place so the ids after it do not shift.
			Loaded.Add(Entry.IsNull() ? nullptr : Entry.LoadSynchronous());
		}
	}
	Rebuild(Loaded);
}

void UMassWarOrderSubsystem::SetOrdersOverride(const TArray<UMassWarOrderDefinition*>& NewDefinitions)
{
	TArray<const UMassWarOrderDefinition*> Converted;
	for (const UMassWarOrderDefinition* Definition : NewDefinitions)
	{
		Converted.Add(Definition);
	}
	Rebuild(Converted);
}

void UMassWarOrderSubsystem::Rebuild(const TArray<const UMassWarOrderDefinition*>& NewDefinitions)
{
	Definitions.Reset();
	Behaviors.Reset();
	Definitions.Add(nullptr); // id 0 = no order
	Behaviors.Add(nullptr);
	for (const UMassWarOrderDefinition* Definition : NewDefinitions)
	{
		if (Definitions.Num() > 255)
		{
			UE_LOG(LogTemp, Error, TEXT("MassWarOrders: more than 255 orders listed - the rest are ignored."));
			break;
		}
		Definitions.Add(Definition);
		UMassWarOrderBehavior* Behavior = nullptr;
		if (Definition)
		{
			if (Definition->Behavior)
			{
				Behavior = NewObject<UMassWarOrderBehavior>(this, Definition->Behavior);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("MassWarOrders: order %s has no Behavior class - it cannot be given."), *Definition->GetName());
			}
		}
		Behaviors.Add(Behavior);
	}
}

uint8 UMassWarOrderSubsystem::FindOrderId(const UMassWarOrderDefinition* Definition) const
{
	if (Definition)
	{
		for (int32 Id = 1; Id < Definitions.Num(); ++Id)
		{
			if (Definitions[Id] == Definition)
			{
				return static_cast<uint8>(Id);
			}
		}
	}
	return 0;
}

FMassWarOrderContext UMassWarOrderSubsystem::MakeContext(FMassEntityManager& EntityManager, FMassEntityHandle Entity, FMassWarCustomOrderFragment& Order) const
{
	FMassWarOrderContext Context;
	Context.EntityManager = &EntityManager;
	Context.Entity = Entity;
	Context.Definition = GetDefinition(Order.OrderId);
	Context.Order = &Order;
	Context.World = GetWorld();
	return Context;
}

bool UMassWarOrderSubsystem::IssueOrder(FMassEntityManager& EntityManager, FMassEntityHandle Entity, const UMassWarOrderDefinition* Definition, const FMassWarOrderRequest& Request, bool bPlayerCommanded)
{
	const uint8 OrderId = FindOrderId(Definition);
	const UMassWarOrderBehavior* Behavior = GetBehavior(OrderId);
	if (OrderId == 0 || !Behavior)
	{
		UE_LOG(LogTemp, Warning, TEXT("MassWarOrders: cannot give %s - it is not listed in Project Settings > MassWar Orders, or has no behaviour."), *GetNameSafe(Definition));
		return false;
	}
	if (!FMassWarUnitStateView::IsLiving(EntityManager, Entity))
	{
		return false;
	}
	FMassWarCustomOrderFragment* Order = EntityManager.GetFragmentDataPtr<FMassWarCustomOrderFragment>(Entity);
	if (!Order || !EntityManager.GetFragmentDataPtr<FMassWarOrderFragment>(Entity))
	{
		return false;
	}

	CancelOrder(EntityManager, Entity, /*bStopUnit=*/ false);

	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	Order->OrderId = OrderId;
	Order->Stage = 0;
	Order->bAwaitingPrimitive = false;
	Order->bBlocksAutoEngage = Definition->bBlocksAutoEngage;
	Order->Location = Request.Location;
	Order->Location2 = Request.Location2;
	Order->Param = Request.Param;
	Order->Target = Request.Target;
	// Spread the ticks of units that got the order together, so they do not all run in the same frame forever.
	Order->NextTickTime = static_cast<float>(Now + Definition->TickInterval * FMath::FRand());

	if (bPlayerCommanded)
	{
		if (FMassWarOrderFragment* Primitive = EntityManager.GetFragmentDataPtr<FMassWarOrderFragment>(Entity))
		{
			Primitive->bPlayerCommanded = Definition->bBlocksAutoEngage;
		}
	}

	Behavior->OnStarted(MakeContext(EntityManager, Entity, *Order));
	return true;
}

void UMassWarOrderSubsystem::CancelOrder(FMassEntityManager& EntityManager, FMassEntityHandle Entity, bool bStopUnit)
{
	FMassWarCustomOrderFragment* Order = EntityManager.IsEntityValid(Entity) ? EntityManager.GetFragmentDataPtr<FMassWarCustomOrderFragment>(Entity) : nullptr;
	if (!Order || !Order->IsActive())
	{
		return;
	}

	if (const UMassWarOrderBehavior* Behavior = GetBehavior(Order->OrderId))
	{
		Behavior->OnEnded(MakeContext(EntityManager, Entity, *Order));
	}
	// OnEnded may have issued something else; only clear if it is still the same ended order.
	Order->Reset();

	if (bStopUnit)
	{
		if (FMassWarOrderFragment* Primitive = EntityManager.GetFragmentDataPtr<FMassWarOrderFragment>(Entity))
		{
			Primitive->OrderType = EMassWarOrderType::Idle;
			Primitive->TargetEntity.Reset();
			Primitive->bPlayerCommanded = false;
		}
	}
}