// Copyright Epic Games, Inc. All Rights Reserved.

#include "Processors/MassWarDamageProcessor.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "Fragments/MassWarUnitFragments.h"
#include "Fragments/MassWarCombatFragments.h"
#include "Registry/MassWarUnitRegistrySubsystem.h"
#include "Engine/World.h"

UMassWarDamageProcessor::UMassWarDamageProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
}

void UMassWarDamageProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassWarOrderFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassWarCombatParamsFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
}

void UMassWarDamageProcessor::InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& EntityManager)
{
	Super::InitializeInternal(Owner, EntityManager);

	CachedEntityManager = EntityManager.ToSharedPtr();

	if (UWorld* World = Owner.GetWorld())
	{
		if (UMassWarUnitRegistrySubsystem* Registry = World->GetSubsystem<UMassWarUnitRegistrySubsystem>())
		{
			Registry->OnDealDamage.BindUObject(this, &UMassWarDamageProcessor::HandleDealDamage);
		}
	}
}

bool UMassWarDamageProcessor::HandleDealDamage(FMassEntityHandle Target, float Damage, FMassEntityHandle Instigator)
{
	if (!CachedEntityManager || !CachedEntityManager->IsEntityValid(Target))
	{
		return false;
	}

	FMassWarHealthFragment* TargetHealth = CachedEntityManager->GetFragmentDataPtr<FMassWarHealthFragment>(Target);
	if (!TargetHealth)
	{
		return false;
	}

	TargetHealth->Health -= Damage;

	UE_LOG(LogTemp, Log, TEXT("MassWarDamageProcessor: (external instigator) dealt %.1f damage to %s, health now %.1f"),
		Damage, *Target.DebugGetDescription(), TargetHealth->Health);

	if (TargetHealth->Health <= 0.f)
	{
		UE_LOG(LogTemp, Log, TEXT("MassWarDamageProcessor: (external instigator) %s died, destroying"), *Target.DebugGetDescription());

		// Deferred, not UMassSpawnerSubsystem::DestroyEntities: this delegate can be invoked from outside
		// any Mass processor (e.g. AMassWarUnitCharacter::TickAttack, a plain Actor tick) while Mass is
		// mid-processing elsewhere on the game thread - DestroyEntities asserts in that case ("called
		// while MassEntity processing in progress"), since it restructures archetype chunks immediately.
		// FMassEntityManager::Defer() is safe to queue from any game-thread context and applies once
		// processing isn't in progress, the same way Context.Defer().DestroyEntity() does from inside a
		// processor's own Execute().
		CachedEntityManager->Defer().DestroyEntity(Target);
	}

	return true;
}

void UMassWarDamageProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const float DeltaTime = Context.GetDeltaTimeSeconds();

	EntityQuery.ForEachEntityChunk(Context, [&EntityManager, DeltaTime](FMassExecutionContext& Context)
	{
		const TArrayView<FMassWarOrderFragment> Orders = Context.GetMutableFragmentView<FMassWarOrderFragment>();
		const TArrayView<FMassWarCombatParamsFragment> CombatParamsList = Context.GetMutableFragmentView<FMassWarCombatParamsFragment>();
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();

		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			FMassWarOrderFragment& Order = Orders[It];
			if (Order.OrderType != EMassWarOrderType::Attack || !Order.TargetEntity.IsValid())
			{
				continue;
			}

			if (!EntityManager.IsEntityValid(Order.TargetEntity))
			{
				Order.OrderType = EMassWarOrderType::Idle;
				Order.TargetEntity.Reset();
				Order.bPlayerCommanded = false;
				continue;
			}

			FMassWarHealthFragment* TargetHealth = EntityManager.GetFragmentDataPtr<FMassWarHealthFragment>(Order.TargetEntity);
			const FTransformFragment* TargetTransform = EntityManager.GetFragmentDataPtr<FTransformFragment>(Order.TargetEntity);
			if (!TargetHealth || !TargetTransform)
			{
				// Target has no health/transform (e.g. MassWarCombat trait not on its config) - nothing to do.
				Order.OrderType = EMassWarOrderType::Idle;
				Order.TargetEntity.Reset();
				Order.bPlayerCommanded = false;
				continue;
			}

			FMassWarCombatParamsFragment& Combat = CombatParamsList[It];
			Combat.TimeSinceLastAttack += DeltaTime;

			const float Distance = FVector::Dist(Transforms[It].GetTransform().GetLocation(), TargetTransform->GetTransform().GetLocation());
			if (Distance > Combat.AttackRange)
			{
				continue;
			}

			if (Combat.TimeSinceLastAttack < Combat.AttackInterval)
			{
				continue;
			}

			Combat.TimeSinceLastAttack = 0.f;
			TargetHealth->Health -= Combat.AttackDamage;

			UE_LOG(LogTemp, Log, TEXT("MassWarDamageProcessor: dealt %.1f damage to %s, health now %.1f"),
				Combat.AttackDamage, *Order.TargetEntity.DebugGetDescription(), TargetHealth->Health);

			if (TargetHealth->Health <= 0.f)
			{
				UE_LOG(LogTemp, Log, TEXT("MassWarDamageProcessor: %s died, destroying"), *Order.TargetEntity.DebugGetDescription());
				Context.Defer().DestroyEntity(Order.TargetEntity);
				Order.OrderType = EMassWarOrderType::Idle;
				Order.TargetEntity.Reset();
				Order.bPlayerCommanded = false;
			}
		}
	});
}
