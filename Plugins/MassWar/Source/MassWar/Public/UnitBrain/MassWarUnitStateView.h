// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityManager.h"
#include "MassCommonFragments.h"
#include "Fragments/MassWarUnitFragments.h"
#include "Interfaces/MassWarUnitStateProviderInterface.h"
#include "MassWarUnitHandle.h"
#include "UObject/Interface.h"

/** Result of FMassWarUnitStateView::FindNearestEnemy - see there. */
struct FMassWarNearestEnemyResult
{
	bool bHasEnemy = false;
	FMassWarUnitHandle NearestEnemy;
	FVector NearestEnemyLocation = FVector::ZeroVector;
	float DistanceToNearestEnemy = 0.f;
};

/**
 * Shared, non-UObject "unit brain" data-access abstraction. AI/gameplay logic (StateTree tasks,
 * conditions, evaluators) is written once against this view instead of touching Mass fragments or
 * Actor components directly, so the exact same logic can run whether a unit is backed by a lightweight
 * Mass entity or (MassWarEmbodiment) a full Actor implementing IMassWarUnitStateProvider.
 *
 * This is a thin, cheap-to-construct wrapper: for the Mass case it holds raw pointers into the entity's
 * own fragments and is only valid for the duration of the call that constructed it (a single StateTree
 * tick, a single processor iteration) - never store it.
 */
class FMassWarUnitStateView
{
public:
	FMassWarUnitStateView() = default;

	FMassWarUnitStateView(FMassEntityManager& InEntityManager, FMassEntityHandle InEntity)
		: EntityManager(&InEntityManager)
		, Entity(InEntity)
	{
	}

	explicit FMassWarUnitStateView(IMassWarUnitStateProvider& InStateProvider)
		: StateProvider(&InStateProvider)
	{
	}

	bool IsValid() const { return (EntityManager != nullptr && Entity.IsValid()) || StateProvider != nullptr; }

	uint8 GetTeam() const
	{
		if (StateProvider)
		{
			return StateProvider->GetMassWarTeamId();
		}
		const FMassWarTeamFragment* Team = GetFragmentPtr<FMassWarTeamFragment>();
		return Team ? Team->TeamId : 0;
	}

	FVector GetLocation() const
	{
		if (StateProvider)
		{
			return StateProvider->GetMassWarLocation();
		}
		const FTransformFragment* Transform = GetFragmentPtr<FTransformFragment>();
		return Transform ? Transform->GetTransform().GetLocation() : FVector::ZeroVector;
	}

	const FMassWarOrderFragment* GetCurrentOrder() const
	{
		return StateProvider ? StateProvider->GetMassWarOrder() : GetFragmentPtr<FMassWarOrderFragment>();
	}

	void RequestMoveTo(const FVector& Destination) const
	{
		if (FMassWarOrderFragment* Order = GetMutableOrder())
		{
			Order->OrderType = EMassWarOrderType::Move;
			Order->Destination = Destination;
			Order->TargetEntity.Reset();
		}
	}

	/**
	 * Attack orders only ever reference a Mass entity target today (FMassWarOrderFragment::TargetEntity
	 * is FMassEntityHandle-typed) - an Actor-embodied Target is accepted here for symmetry with
	 * FindNearestEnemy's results, but silently produces no order (documented Pass 7 scope cut: ordinary
	 * Mass units can't target/damage an embodied hero yet).
	 */
	void RequestAttack(const FMassWarUnitHandle& Target) const
	{
		if (!Target.IsMassEntity())
		{
			return;
		}
		if (FMassWarOrderFragment* Order = GetMutableOrder())
		{
			Order->OrderType = EMassWarOrderType::Attack;
			Order->TargetEntity = Target.Entity;
		}
	}

	FMassEntityManager* GetEntityManager() const { return EntityManager; }
	FMassEntityHandle GetEntity() const { return Entity; }

	/** Resolves a unit handle (either backing) into a view, so callers can inspect a *candidate* unit
	 *  (not just "self") the same way - see FindNearestEnemy.
	 *
	 *  FMassEntityHandle::IsValid() only checks the handle is structurally well-formed (non-zero
	 *  index/serial) - it says nothing about whether that entity is still alive. Registry candidate lists
	 *  (UMassWarUnitRegistrySubsystem::GetAllUnitHandles) can and do contain handles for units that have
	 *  since died, so this must confirm liveness with the EntityManager itself before treating the handle
	 *  as a real candidate - skipping this crashed FindNearestEnemy on a dangling fragment read as soon as
	 *  any candidate died mid-search. */
	static FMassWarUnitStateView FromHandle(FMassEntityManager& EntityManager, const FMassWarUnitHandle& Handle)
	{
		if (Handle.IsMassEntity())
		{
			if (!EntityManager.IsEntityValid(Handle.Entity))
			{
				return FMassWarUnitStateView();
			}
			return FMassWarUnitStateView(EntityManager, Handle.Entity);
		}
		if (AActor* Actor = Handle.Actor.Get())
		{
			if (IMassWarUnitStateProvider* Provider = Cast<IMassWarUnitStateProvider>(Actor))
			{
				return FMassWarUnitStateView(*Provider);
			}
		}
		return FMassWarUnitStateView();
	}

	/**
	 * Shared "find nearest enemy" logic used by both the Mass and Actor StateTree evaluator variants
	 * (FMassWarSTEval_FindNearestEnemy / ...Actor) - Candidates is expected to be every known unit
	 * (UMassWarUnitRegistrySubsystem::GetAllUnitHandles), Mass entities and embodied Actors alike.
	 */
	static FMassWarNearestEnemyResult FindNearestEnemy(
		FMassEntityManager& EntityManager,
		uint8 SelfTeam,
		const FVector& SelfLocation,
		const FMassWarUnitHandle& SelfHandle,
		TArrayView<const FMassWarUnitHandle> Candidates,
		float SearchRadius)
	{
		FMassWarNearestEnemyResult Result;
		float BestDistSq = FMath::Square(SearchRadius);

		for (const FMassWarUnitHandle& Candidate : Candidates)
		{
			if (Candidate == SelfHandle)
			{
				continue;
			}

			const FMassWarUnitStateView CandidateView = FromHandle(EntityManager, Candidate);
			if (!CandidateView.IsValid())
			{
				continue;
			}

			if (MassWarGetAffiliation(SelfTeam, CandidateView.GetTeam()) != EMassWarAffiliation::Enemy)
			{
				continue;
			}

			const FVector CandidateLocation = CandidateView.GetLocation();
			const float DistSq = FVector::DistSquared(SelfLocation, CandidateLocation);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Result.bHasEnemy = true;
				Result.NearestEnemy = Candidate;
				Result.NearestEnemyLocation = CandidateLocation;
				Result.DistanceToNearestEnemy = FMath::Sqrt(DistSq);
			}
		}

		return Result;
	}

private:
	template<typename T>
	const T* GetFragmentPtr() const
	{
		return EntityManager ? EntityManager->GetFragmentDataPtr<T>(Entity) : nullptr;
	}

	FMassWarOrderFragment* GetMutableOrder() const
	{
		return StateProvider ? StateProvider->GetMassWarOrderMutable() : (EntityManager ? EntityManager->GetFragmentDataPtr<FMassWarOrderFragment>(Entity) : nullptr);
	}

	FMassEntityManager* EntityManager = nullptr;
	FMassEntityHandle Entity;
	IMassWarUnitStateProvider* StateProvider = nullptr;
};
