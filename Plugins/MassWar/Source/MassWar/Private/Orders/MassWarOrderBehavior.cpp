// Copyright Epic Games, Inc. All Rights Reserved.

#include "Orders/MassWarOrderBehavior.h"
#include "Orders/MassWarOrderDefinition.h"
#include "Fragments/MassWarUnitFragments.h"
#include "Stats/MassWarStatSubsystem.h"
#include "UnitBrain/MassWarUnitStateView.h"
#include "MassCommonFragments.h"
#include "MassEntityManager.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassWarOrderBehavior)

void UMassWarOrderLibrary::MoveTo(const FMassWarOrderContext& Context, FVector Location, bool bStopAtAttackDistance)
{
	if (!Context.IsValid())
	{
		return;
	}
	if (FMassWarOrderFragment* Primitive = Context.EntityManager->GetFragmentDataPtr<FMassWarOrderFragment>(Context.Entity))
	{
		Primitive->OrderType = EMassWarOrderType::Move;
		Primitive->Destination = Location;
		Primitive->TargetEntity.Reset();
		Primitive->bStopAtAttackDistance = bStopAtAttackDistance;
		Primitive->bPlayerCommanded = Context.Order->bBlocksAutoEngage;
		Context.Order->bAwaitingPrimitive = true;
	}
}

bool UMassWarOrderLibrary::AttackOrderTarget(const FMassWarOrderContext& Context)
{
	if (!Context.IsValid() || !FMassWarUnitStateView::IsLiving(*Context.EntityManager, Context.Order->Target))
	{
		return false;
	}
	if (FMassWarOrderFragment* Primitive = Context.EntityManager->GetFragmentDataPtr<FMassWarOrderFragment>(Context.Entity))
	{
		Primitive->OrderType = EMassWarOrderType::Attack;
		Primitive->TargetEntity = Context.Order->Target;
		Primitive->bPlayerCommanded = Context.Order->bBlocksAutoEngage;
		Context.Order->bAwaitingPrimitive = true;
		return true;
	}
	return false;
}

void UMassWarOrderLibrary::Stop(const FMassWarOrderContext& Context)
{
	if (!Context.IsValid())
	{
		return;
	}
	if (FMassWarOrderFragment* Primitive = Context.EntityManager->GetFragmentDataPtr<FMassWarOrderFragment>(Context.Entity))
	{
		Primitive->OrderType = EMassWarOrderType::Idle;
		Primitive->TargetEntity.Reset();
		Primitive->bPlayerCommanded = false;
	}
	Context.Order->bAwaitingPrimitive = false;
}

void UMassWarOrderLibrary::Finish(const FMassWarOrderContext& Context)
{
	if (!Context.IsValid())
	{
		return;
	}
	Stop(Context);
	Context.Order->Reset();
}

FVector UMassWarOrderLibrary::GetUnitLocation(const FMassWarOrderContext& Context)
{
	const FTransformFragment* Transform = Context.IsValid() ? Context.EntityManager->GetFragmentDataPtr<FTransformFragment>(Context.Entity) : nullptr;
	return Transform ? Transform->GetTransform().GetLocation() : FVector::ZeroVector;
}

FVector UMassWarOrderLibrary::GetOrderLocation(const FMassWarOrderContext& Context)
{
	return Context.Order ? Context.Order->Location : FVector::ZeroVector;
}

FVector UMassWarOrderLibrary::GetOrderLocation2(const FMassWarOrderContext& Context)
{
	return Context.Order ? Context.Order->Location2 : FVector::ZeroVector;
}

float UMassWarOrderLibrary::GetOrderParam(const FMassWarOrderContext& Context)
{
	return Context.Order ? Context.Order->Param : 0.f;
}

float UMassWarOrderLibrary::GetDistanceToLocation(const FMassWarOrderContext& Context, FVector Location)
{
	return static_cast<float>(FVector::Dist2D(GetUnitLocation(Context), Location));
}

int32 UMassWarOrderLibrary::GetStage(const FMassWarOrderContext& Context)
{
	return Context.Order ? Context.Order->Stage : 0;
}

void UMassWarOrderLibrary::SetStage(const FMassWarOrderContext& Context, int32 Stage)
{
	if (Context.Order)
	{
		Context.Order->Stage = static_cast<uint8>(FMath::Clamp(Stage, 0, 255));
	}
}

bool UMassWarOrderLibrary::IsOrderTargetAlive(const FMassWarOrderContext& Context)
{
	return Context.IsValid() && FMassWarUnitStateView::IsLiving(*Context.EntityManager, Context.Order->Target);
}

FVector UMassWarOrderLibrary::GetOrderTargetLocation(const FMassWarOrderContext& Context)
{
	const FTransformFragment* Transform = (Context.IsValid() && Context.EntityManager->IsEntityValid(Context.Order->Target))
		? Context.EntityManager->GetFragmentDataPtr<FTransformFragment>(Context.Order->Target) : nullptr;
	return Transform ? Transform->GetTransform().GetLocation() : FVector::ZeroVector;
}

float UMassWarOrderLibrary::GetUnitStat(const FMassWarOrderContext& Context, FGameplayTag StatTag)
{
	const UMassWarStatSubsystem* Stats = Context.World ? Context.World->GetSubsystem<UMassWarStatSubsystem>() : nullptr;
	return (Stats && Context.IsValid()) ? Stats->GetValue(*Context.EntityManager, Context.Entity, StatTag) : 0.f;
}