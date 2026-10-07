// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityManager.h"
#include "MassCommonFragments.h"
#include "Fragments/MassWarUnitFragments.h"
#include "MassWarUnitHandle.h"

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
 * conditions, evaluators) is written against this view instead of touching Mass fragments directly.
 *
 * This is a thin, cheap-to-construct wrapper holding raw pointers into the entity manager: it's only
 * valid for the duration of the call that constructed it (a single StateTree tick, a single processor
 * iteration) - never store it.
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

	bool IsValid() const { return EntityManager != nullptr && Entity.IsValid(); }

	uint8 GetTeam() const
	{
		const FMassWarTeamFragment* Team = GetFragmentPtr<FMassWarTeamFragment>();
		return Team ? Team->TeamId : 0;
	}

	FVector GetLocation() const
	{
		const FTransformFragment* Transform = GetFragmentPtr<FTransformFragment>();
		return Transform ? Transform->GetTransform().GetLocation() : FVector::ZeroVector;
	}

	const FMassWarOrderFragment* GetCurrentOrder() const
	{
		return GetFragmentPtr<FMassWarOrderFragment>();
	}

	/** bStopAtAttackDistance: complete the move once within AttackStopDistance of Destination instead of the
	 *  (small) AcceptanceRadius - see FMassWarOrderFragment::bStopAtAttackDistance. */
	void RequestMoveTo(const FVector& Destination, bool bStopAtAttackDistance = false) const
	{
		if (FMassWarOrderFragment* Order = GetMutableOrder())
		{
			Order->OrderType = EMassWarOrderType::Move;
			Order->Destination = Destination;
			Order->TargetEntity.Reset();
			Order->bStopAtAttackDistance = bStopAtAttackDistance;
		}
	}

	void RequestAttack(const FMassWarUnitHandle& Target) const
	{
		if (!Target.IsValid())
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

	/** Resolves a unit handle into a view, so callers can inspect a *candidate* unit (not just "self") the
	 *  same way - see FindNearestEnemy. Returns an invalid view for a dead OR dying unit.
	 *
	 *  FMassEntityHandle::IsValid() only checks the handle is structurally well-formed (non-zero
	 *  index/serial) - it says nothing about whether that entity is still alive. Registry candidate lists
	 *  (UMassWarUnitRegistrySubsystem::GetAllUnitHandles) can and do contain handles for units that have
	 *  since died, so this must confirm liveness with the EntityManager itself before treating the handle
	 *  as a real candidate - skipping this crashed FindNearestEnemy on a dangling fragment read as soon as
	 *  any candidate died mid-search. */
	static FMassWarUnitStateView FromHandle(FMassEntityManager& EntityManager, const FMassWarUnitHandle& Handle)
	{
		// A dying unit is out of the game already (see FMassWarLifeFragment): it must not be found as a
		// target, chased, or attacked - it only lingers so its visual can play the death animation.
		if (!Handle.IsValid() || !IsLiving(EntityManager, Handle.Entity))
		{
			return FMassWarUnitStateView();
		}
		return FMassWarUnitStateView(EntityManager, Handle.Entity);
	}

	/** True if Entity still exists AND is not dying - the check gameplay code wants before treating a unit
	 *  as selectable, orderable or targetable. (FMassEntityManager::IsEntityValid alone stays true for a
	 *  dying unit until it is finally destroyed.) */
	static bool IsLiving(FMassEntityManager& EntityManager, FMassEntityHandle Entity)
	{
		if (!EntityManager.IsEntityValid(Entity))
		{
			return false;
		}
		const FMassWarLifeFragment* Life = EntityManager.GetFragmentDataPtr<FMassWarLifeFragment>(Entity);
		return !(Life && Life->IsDying());
	}

	/**
	 * Shared "find nearest enemy" logic used by the StateTree evaluator - Candidates is expected to be
	 * every known unit (UMassWarUnitRegistrySubsystem::GetAllUnitHandles).
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
		return EntityManager ? EntityManager->GetFragmentDataPtr<FMassWarOrderFragment>(Entity) : nullptr;
	}

	FMassEntityManager* EntityManager = nullptr;
	FMassEntityHandle Entity;
};
