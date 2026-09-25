// Copyright Epic Games, Inc. All Rights Reserved.

#include "Processors/MassWarDamageProcessor.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "Fragments/MassWarUnitFragments.h"
#include "Fragments/MassWarCombatFragments.h"
#include "UnitBrain/MassWarUnitStateView.h"
#include "Fragments/MassWarPerceptionFragments.h"
#include "Perception/MassWarPerceptionSubsystem.h"
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
	EntityQuery.AddRequirement<FMassWarLifeFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassWarAttackFeedbackFragment>(EMassFragmentAccess::ReadWrite);
	// Not read here - declared because a landed attack writes the victim's perception (see ReportDamage), which tells
	// Mass this processor must not run at the same time as anything else touching perception data.
	EntityQuery.AddRequirement<FMassWarPerceptionFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::Optional);
}

void UMassWarDamageProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const float DeltaTime = Context.GetDeltaTimeSeconds();
	UWorld* World = GetWorld();
	UMassWarPerceptionSubsystem* PerceptionSubsystem = World ? World->GetSubsystem<UMassWarPerceptionSubsystem>() : nullptr;

	EntityQuery.ForEachEntityChunk(Context, [&EntityManager, DeltaTime, PerceptionSubsystem](FMassExecutionContext& Context)
	{
		const TArrayView<FMassWarOrderFragment> Orders = Context.GetMutableFragmentView<FMassWarOrderFragment>();
		const TArrayView<FMassWarCombatParamsFragment> CombatParamsList = Context.GetMutableFragmentView<FMassWarCombatParamsFragment>();
		const TConstArrayView<FTransformFragment> Transforms = Context.GetFragmentView<FTransformFragment>();
		const TConstArrayView<FMassWarLifeFragment> LifeList = Context.GetFragmentView<FMassWarLifeFragment>();
		const TArrayView<FMassWarAttackFeedbackFragment> AttackFeedbackList = Context.GetMutableFragmentView<FMassWarAttackFeedbackFragment>();

		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			FMassWarOrderFragment& Order = Orders[It];

			// A dying unit is out of the fight: whatever it was doing stops with it.
			if (LifeList[It].IsDying())
			{
				Order.OrderType = EMassWarOrderType::Idle;
				Order.TargetEntity.Reset();
				Order.bPlayerCommanded = false;
				continue;
			}

			if (Order.OrderType != EMassWarOrderType::Attack || !Order.TargetEntity.IsValid())
			{
				continue;
			}

			// An invalid view means the target is destroyed - or dying, which is just as gone for combat.
			const FMassWarUnitStateView TargetView = FMassWarUnitStateView::FromHandle(EntityManager, FMassWarUnitHandle(Order.TargetEntity));
			if (!TargetView.IsValid())
			{
				Order.OrderType = EMassWarOrderType::Idle;
				Order.TargetEntity.Reset();
				Order.bPlayerCommanded = false;
				continue;
			}

			FMassWarHealthFragment* TargetHealth = EntityManager.GetFragmentDataPtr<FMassWarHealthFragment>(Order.TargetEntity);
			if (!TargetHealth)
			{
				// Target has no health (e.g. MassWarCombat trait not on its config) - nothing to do.
				Order.OrderType = EMassWarOrderType::Idle;
				Order.TargetEntity.Reset();
				Order.bPlayerCommanded = false;
				continue;
			}

			FMassWarCombatParamsFragment& Combat = CombatParamsList[It];
			Combat.TimeSinceLastAttack += DeltaTime;

			const float Distance = FVector::Dist(Transforms[It].GetTransform().GetLocation(), TargetView.GetLocation());
			if (Distance > Combat.AttackRange)
			{
				continue;
			}

			if (Combat.TimeSinceLastAttack < Combat.AttackInterval)
			{
				continue;
			}

			Combat.TimeSinceLastAttack = 0.f;
			++AttackFeedbackList[It].AttackCounter; // wraps; lets every machine's puppet play an attack animation
			TargetHealth->Health -= Combat.AttackDamage;

			// Tell perception: the victim now knows who hit it and from where, and enemies nearby hear the attack.
			if (PerceptionSubsystem)
			{
				const FMassEntityHandle Attacker = Context.GetEntity(It);
				const FVector AttackerLocation = Transforms[It].GetTransform().GetLocation();
				PerceptionSubsystem->ReportDamage(EntityManager, Order.TargetEntity, Attacker, AttackerLocation, Combat.AttackDamage);
				if (Combat.AttackNoiseRange > 0.f)
				{
					const FMassWarTeamFragment* AttackerTeam = EntityManager.GetFragmentDataPtr<FMassWarTeamFragment>(Attacker);
					PerceptionSubsystem->ReportNoise(AttackerLocation, Combat.AttackNoiseRange, Attacker, AttackerTeam ? AttackerTeam->TeamId : 0);
				}
			}

			UE_LOG(LogTemp, Log, TEXT("MassWarDamageProcessor: dealt %.1f damage to %s, health now %.1f"),
				Combat.AttackDamage, *Order.TargetEntity.DebugGetDescription(), TargetHealth->Health);

			if (TargetHealth->Health <= 0.f)
			{
				if (FMassWarLifeFragment* TargetLife = EntityManager.GetFragmentDataPtr<FMassWarLifeFragment>(Order.TargetEntity))
				{
					UE_LOG(LogTemp, Log, TEXT("MassWarDamageProcessor: %s died, now dying for %.1fs"), *Order.TargetEntity.DebugGetDescription(), TargetLife->LingerTime);

					// Don't destroy: the entity lingers as Dying so its visual can play a death animation
					// (UMassWarDeathProcessor removes it afterwards). The dead unit's own order is dropped now.
					TargetLife->State = EMassWarLifeState::Dying;
					TargetLife->TimeDying = 0.f;
					if (FMassWarOrderFragment* TargetOrder = EntityManager.GetFragmentDataPtr<FMassWarOrderFragment>(Order.TargetEntity))
					{
						TargetOrder->OrderType = EMassWarOrderType::Idle;
						TargetOrder->TargetEntity.Reset();
						TargetOrder->bPlayerCommanded = false;
					}
				}
				else
				{
					// A unit without the Core life fragment can't linger - remove it outright, as before.
					UE_LOG(LogTemp, Log, TEXT("MassWarDamageProcessor: %s died, destroying"), *Order.TargetEntity.DebugGetDescription());
					Context.Defer().DestroyEntity(Order.TargetEntity);
				}

				Order.OrderType = EMassWarOrderType::Idle;
				Order.TargetEntity.Reset();
				Order.bPlayerCommanded = false;
			}
		}
	});
}
