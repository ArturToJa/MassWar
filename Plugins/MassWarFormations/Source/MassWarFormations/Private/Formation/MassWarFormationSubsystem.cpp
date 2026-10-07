// Copyright Epic Games, Inc. All Rights Reserved.

#include "Formation/MassWarFormationSubsystem.h"
#include "Formation/MassWarFormationLayout.h"
#include "MassEntityManager.h"
#include "MassEntityUtils.h"
#include "MassCommonFragments.h"
#include "Fragments/MassWarUnitFragments.h"
#include "UnitBrain/MassWarUnitStateView.h"
#include "Engine/World.h"
#include "Algo/Sort.h"

namespace
{
	FVector RightOf(const FVector& Forward)
	{
		return FVector(-Forward.Y, Forward.X, 0.0);
	}
}

uint32 UMassWarFormationSubsystem::CreateFormation(FMassEntityManager& EntityManager, TConstArrayView<FMassEntityHandle> Members, uint32 OwnerPlayerId, const FMassWarFormationSettings& Settings)
{
	FMassWarFormation Formation;
	Formation.Id = NextFormationId++;
	Formation.OwnerPlayerId = OwnerPlayerId;
	Formation.Settings = Settings;

	for (const FMassEntityHandle& Member : Members)
	{
		if (!FMassWarUnitStateView::IsLiving(EntityManager, Member))
		{
			continue;
		}
		FMassWarFormationMemberFragment* MemberFragment = EntityManager.GetFragmentDataPtr<FMassWarFormationMemberFragment>(Member);
		if (!MemberFragment)
		{
			continue;
		}
		// Leaving an old formation: it notices on its next maintenance pass (the member no longer points at it).
		MemberFragment->FormationId = Formation.Id;
		Formation.Members.Add(Member);
		if (const FMassWarTeamFragment* Team = EntityManager.GetFragmentDataPtr<FMassWarTeamFragment>(Member))
		{
			Formation.TeamId = Team->TeamId;
		}
	}

	const uint32 Id = Formation.Id;
	Refresh(EntityManager, Formation);
	Formations.Add(Id, MoveTemp(Formation));
	return Id;
}

bool UMassWarFormationSubsystem::Refresh(FMassEntityManager& EntityManager, FMassWarFormation& Formation) const
{
	FVector Sum = FVector::ZeroVector;
	int32 Count = 0;
	for (int32 Index = Formation.Members.Num() - 1; Index >= 0; --Index)
	{
		const FMassEntityHandle Member = Formation.Members[Index];
		const FMassWarFormationMemberFragment* MemberFragment = FMassWarUnitStateView::IsLiving(EntityManager, Member)
			? EntityManager.GetFragmentDataPtr<FMassWarFormationMemberFragment>(Member) : nullptr;
		if (!MemberFragment || MemberFragment->FormationId != Formation.Id)
		{
			Formation.Members.RemoveAtSwap(Index);
			continue;
		}
		if (const FTransformFragment* Transform = EntityManager.GetFragmentDataPtr<FTransformFragment>(Member))
		{
			Sum += Transform->GetTransform().GetLocation();
			++Count;
		}
	}
	if (Count > 0)
	{
		Formation.Anchor = Sum / Count;
	}
	return Formation.Members.Num() > 0;
}

void UMassWarFormationSubsystem::ApplyMove(FMassEntityManager& EntityManager, FMassWarFormation& Formation, const FVector& SlotCentre, const FVector& Forward) const
{
	const int32 Count = Formation.Members.Num();
	if (Count == 0)
	{
		return;
	}

	TArray<FVector2D> Offsets;
	MassWarFormationLayout::ComputeSlotOffsets(Formation.Settings, Count, Offsets);

	const FVector Right = RightOf(Forward);
	TArray<FVector> Slots;
	Slots.Reserve(Count);
	for (const FVector2D& Offset : Offsets)
	{
		Slots.Add(SlotCentre + Forward * Offset.X + Right * Offset.Y);
	}

	// Give each slot - front ranks first - to the nearest member still without one, so members take the slot
	// closest to where they already are and paths cross as little as possible.
	TArray<int32> SlotOrder;
	SlotOrder.Reserve(Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		SlotOrder.Add(Index);
	}
	SlotOrder.Sort([&Offsets](int32 A, int32 B) { return Offsets[A].X > Offsets[B].X; });

	TArray<FVector> MemberLocations;
	MemberLocations.Reserve(Count);
	for (const FMassEntityHandle& Member : Formation.Members)
	{
		const FTransformFragment* Transform = EntityManager.GetFragmentDataPtr<FTransformFragment>(Member);
		MemberLocations.Add(Transform ? Transform->GetTransform().GetLocation() : Formation.Anchor);
	}

	TBitArray<> Taken(false, Count);
	for (const int32 SlotIndex : SlotOrder)
	{
		int32 Best = INDEX_NONE;
		double BestDistSq = TNumericLimits<double>::Max();
		for (int32 MemberIndex = 0; MemberIndex < Count; ++MemberIndex)
		{
			if (Taken[MemberIndex])
			{
				continue;
			}
			const double DistSq = FVector::DistSquared2D(MemberLocations[MemberIndex], Slots[SlotIndex]);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Best = MemberIndex;
			}
		}
		Taken[Best] = true;

		if (FMassWarOrderFragment* Order = EntityManager.GetFragmentDataPtr<FMassWarOrderFragment>(Formation.Members[Best]))
		{
			Order->OrderType = EMassWarOrderType::Move;
			Order->Destination = Slots[SlotIndex];
			Order->TargetEntity.Reset();
			Order->bStopAtAttackDistance = false; // every member goes all the way to its slot
			Order->bPlayerCommanded = true;
		}
	}

	Formation.OrderType = EMassWarFormationOrderType::Move;
	Formation.TargetFormationId = 0;
}

void UMassWarFormationSubsystem::IssueMoveOrder(FMassEntityManager& EntityManager, TConstArrayView<uint32> FormationIds, const FVector& Destination, uint32 RequesterPlayerId)
{
	TArray<FMassWarFormation*> Selected;
	FVector Centroid = FVector::ZeroVector;
	for (const uint32 Id : FormationIds)
	{
		FMassWarFormation* Formation = Formations.Find(Id);
		if (!Formation || Formation->OwnerPlayerId != RequesterPlayerId || Selected.Contains(Formation) || !Refresh(EntityManager, *Formation))
		{
			continue;
		}
		Selected.Add(Formation);
		Centroid += Formation->Anchor;
	}
	if (Selected.IsEmpty())
	{
		return;
	}
	Centroid /= Selected.Num();

	// All formations face the way the group as a whole is going.
	FVector Forward = (Destination - Centroid).GetSafeNormal2D();
	if (Forward.IsNearlyZero())
	{
		Forward = FVector::ForwardVector;
	}
	const FVector Right = RightOf(Forward);

	// Side by side, keeping the left-to-right order they already have so they don't cross over.
	Algo::Sort(Selected, [&](const FMassWarFormation* A, const FMassWarFormation* B)
	{
		return FVector::DotProduct(A->Anchor - Centroid, Right) < FVector::DotProduct(B->Anchor - Centroid, Right);
	});

	float TotalWidth = FormationGap * (Selected.Num() - 1);
	for (const FMassWarFormation* Formation : Selected)
	{
		TotalWidth += 2.f * MassWarFormationLayout::ComputeHalfWidth(Formation->Settings, Formation->Members.Num());
	}
	float Cursor = -TotalWidth * 0.5f;
	for (FMassWarFormation* Formation : Selected)
	{
		const float Width = 2.f * MassWarFormationLayout::ComputeHalfWidth(Formation->Settings, Formation->Members.Num());
		ApplyMove(EntityManager, *Formation, Destination + Right * (Cursor + Width * 0.5f), Forward);
		Cursor += Width + FormationGap;
	}
}

void UMassWarFormationSubsystem::IssueAttackOrder(FMassEntityManager& EntityManager, TConstArrayView<uint32> FormationIds, uint32 TargetFormationId, uint32 RequesterPlayerId)
{
	FMassWarFormation* Target = Formations.Find(TargetFormationId);
	if (!Target || !Refresh(EntityManager, *Target))
	{
		return;
	}

	for (const uint32 Id : FormationIds)
	{
		FMassWarFormation* Formation = Formations.Find(Id);
		if (!Formation || Formation == Target || Formation->OwnerPlayerId != RequesterPlayerId || !Refresh(EntityManager, *Formation)
			|| MassWarGetAffiliation(Formation->TeamId, Target->TeamId) != EMassWarAffiliation::Enemy)
		{
			continue;
		}
		Formation->OrderType = EMassWarFormationOrderType::Attack;
		Formation->TargetFormationId = TargetFormationId;
		AssignTargets(EntityManager, *Formation, *Target);
	}
}

void UMassWarFormationSubsystem::AssignTargets(FMassEntityManager& EntityManager, FMassWarFormation& Formation, const FMassWarFormation& Target) const
{
	// Living enemies, how many of our members already go for each, and who still needs a target.
	TArray<FMassEntityHandle> Enemies;
	for (const FMassEntityHandle& Enemy : Target.Members)
	{
		if (FMassWarUnitStateView::IsLiving(EntityManager, Enemy))
		{
			Enemies.Add(Enemy);
		}
	}
	if (Enemies.IsEmpty())
	{
		return;
	}

	TArray<int32> Load;
	Load.SetNumZeroed(Enemies.Num());
	TArray<FMassEntityHandle> NeedTarget;
	for (const FMassEntityHandle& Member : Formation.Members)
	{
		const FMassWarOrderFragment* Order = EntityManager.GetFragmentDataPtr<FMassWarOrderFragment>(Member);
		const int32 CurrentTarget = (Order && Order->OrderType == EMassWarOrderType::Attack) ? Enemies.IndexOfByKey(Order->TargetEntity) : INDEX_NONE;
		if (CurrentTarget != INDEX_NONE)
		{
			++Load[CurrentTarget];
		}
		else
		{
			NeedTarget.Add(Member);
		}
	}

	const int32 Capacity = FMath::DivideAndRoundUp(Formation.Members.Num(), Enemies.Num());
	for (const FMassEntityHandle& Member : NeedTarget)
	{
		const FTransformFragment* Transform = EntityManager.GetFragmentDataPtr<FTransformFragment>(Member);
		const FVector From = Transform ? Transform->GetTransform().GetLocation() : Formation.Anchor;

		int32 Best = INDEX_NONE;
		double BestDistSq = TNumericLimits<double>::Max();
		for (int32 EnemyIndex = 0; EnemyIndex < Enemies.Num(); ++EnemyIndex)
		{
			if (Load[EnemyIndex] >= Capacity)
			{
				continue;
			}
			const FTransformFragment* EnemyTransform = EntityManager.GetFragmentDataPtr<FTransformFragment>(Enemies[EnemyIndex]);
			const double DistSq = EnemyTransform ? FVector::DistSquared(From, EnemyTransform->GetTransform().GetLocation()) : TNumericLimits<double>::Max();
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Best = EnemyIndex;
			}
		}
		if (Best == INDEX_NONE)
		{
			Best = 0; // every enemy is at capacity (rounding): just take the first
		}

		if (FMassWarOrderFragment* Order = EntityManager.GetFragmentDataPtr<FMassWarOrderFragment>(Member))
		{
			Order->OrderType = EMassWarOrderType::Attack;
			Order->TargetEntity = Enemies[Best];
			Order->bPlayerCommanded = true;
		}
		++Load[Best];
	}
}

void UMassWarFormationSubsystem::PoolKnowledge(FMassEntityManager& EntityManager, FMassWarFormation& Formation, double Now) const
{
	// Start from what the formation already knew (minus the dead and the forgotten - and nothing is "in sight"
	// until a member says so again), then fold in every member's own perception.
	TMap<FMassEntityHandle, FMassWarPerceivedEntry> Pooled;
	for (FMassWarPerceivedEntry Known : Formation.Knowledge)
	{
		if (Now - Known.LastStimulusTime <= Formation.Settings.KnowledgeMemory && FMassWarUnitStateView::IsLiving(EntityManager, Known.Entity))
		{
			Known.bSeenNow = false;
			Pooled.Add(Known.Entity, Known);
		}
	}

	for (const FMassEntityHandle& Member : Formation.Members)
	{
		const FMassWarPerceptionFragment* Perception = EntityManager.GetFragmentDataPtr<FMassWarPerceptionFragment>(Member);
		if (!Perception)
		{
			continue;
		}
		for (const FMassWarPerceivedEntry& Entry : Perception->GetEntries())
		{
			if (!FMassWarUnitStateView::IsLiving(EntityManager, Entry.Entity))
			{
				continue;
			}
			FMassWarPerceivedEntry* Known = Pooled.Find(Entry.Entity);
			if (!Known)
			{
				Pooled.Add(Entry.Entity, Entry);
				continue;
			}
			if (Entry.LastStimulusTime > Known->LastStimulusTime)
			{
				const bool bSeen = Known->bSeenNow;
				const double DamageTime = Known->LastDamageTime;
				const double HeardTime = Known->LastHeardTime;
				*Known = Entry;
				Known->bSeenNow = bSeen;
				Known->LastDamageTime = FMath::Max(DamageTime, Entry.LastDamageTime);
				Known->LastHeardTime = FMath::Max(HeardTime, Entry.LastHeardTime);
			}
			else
			{
				Known->LastDamageTime = FMath::Max(Known->LastDamageTime, Entry.LastDamageTime);
				Known->LastHeardTime = FMath::Max(Known->LastHeardTime, Entry.LastHeardTime);
			}
			Known->bSeenNow |= Entry.bSeenNow;
		}
	}

	Pooled.GenerateValueArray(Formation.Knowledge);
	if (Formation.Knowledge.Num() > FMassWarFormation::MaxKnowledge)
	{
		Algo::Sort(Formation.Knowledge, [](const FMassWarPerceivedEntry& A, const FMassWarPerceivedEntry& B) { return A.LastStimulusTime > B.LastStimulusTime; });
		Formation.Knowledge.SetNum(FMassWarFormation::MaxKnowledge);
	}
}

void UMassWarFormationSubsystem::AutoBehave(FMassEntityManager& EntityManager, FMassWarFormation& Formation, double Now)
{
	// 1) An enemy formation is in sight of any member and close enough: attack it.
	if (Formation.Settings.bAutoEngage)
	{
		const FMassWarPerceivedEntry* Best = MassWarFindBestPerceived(Formation.Knowledge, Formation.Anchor, Now, /*bIncludeRemembered=*/ false, 0.f);
		if (Best && FVector::Dist2D(Formation.Anchor, FVector(Best->LastKnownLocation)) <= Formation.Settings.AutoEngageRadius)
		{
			const FMassWarFormationMemberFragment* EnemyMember = EntityManager.GetFragmentDataPtr<FMassWarFormationMemberFragment>(Best->Entity);
			FMassWarFormation* Target = EnemyMember ? Formations.Find(EnemyMember->FormationId) : nullptr;
			if (Target && Target != &Formation && Target->Members.Num() > 0
				&& MassWarGetAffiliation(Formation.TeamId, Target->TeamId) == EMassWarAffiliation::Enemy)
			{
				Formation.OrderType = EMassWarFormationOrderType::Attack;
				Formation.TargetFormationId = Target->Id;
				AssignTargets(EntityManager, Formation, *Target);
				return;
			}
		}
	}

	// 2) Nothing in sight, but a member heard something or was hit lately: go and look, once per event.
	if (Formation.Settings.bInvestigate)
	{
		const FMassWarPerceivedEntry* Newest = nullptr;
		double NewestTime = Formation.LastInvestigatedTime;
		for (const FMassWarPerceivedEntry& Known : Formation.Knowledge)
		{
			const double StimulusTime = FMath::Max(Known.LastHeardTime, Known.LastDamageTime);
			if (!Known.bSeenNow && StimulusTime > NewestTime && Now - StimulusTime <= 3.0)
			{
				Newest = &Known;
				NewestTime = StimulusTime;
			}
		}
		if (Newest)
		{
			Formation.LastInvestigatedTime = NewestTime;
			const FVector Spot(Newest->LastKnownLocation);
			FVector Forward = (Spot - Formation.Anchor).GetSafeNormal2D();
			if (Forward.IsNearlyZero())
			{
				Forward = FVector::ForwardVector;
			}
			ApplyMove(EntityManager, Formation, Spot, Forward);
		}
	}
}

void UMassWarFormationSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client || Formations.IsEmpty())
	{
		return;
	}
	TimeSinceMaintenance += DeltaTime;
	if (TimeSinceMaintenance < MaintenanceInterval)
	{
		return;
	}
	TimeSinceMaintenance = 0.f;

	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*World);
	const double Now = World->GetTimeSeconds();

	TArray<uint32> Empty;
	for (TPair<uint32, FMassWarFormation>& Pair : Formations)
	{
		FMassWarFormation& Formation = Pair.Value;
		if (!Refresh(EntityManager, Formation))
		{
			Empty.Add(Pair.Key);
			continue;
		}

		if (Formation.OrderType == EMassWarFormationOrderType::Move)
		{
			bool bAnyStillMoving = false;
			for (const FMassEntityHandle& Member : Formation.Members)
			{
				const FMassWarOrderFragment* Order = EntityManager.GetFragmentDataPtr<FMassWarOrderFragment>(Member);
				if (Order && Order->OrderType == EMassWarOrderType::Move)
				{
					bAnyStillMoving = true;
					break;
				}
			}
			if (!bAnyStillMoving)
			{
				Formation.OrderType = EMassWarFormationOrderType::Idle;
			}
		}
		else if (Formation.OrderType == EMassWarFormationOrderType::Attack)
		{
			const FMassWarFormation* Target = Formations.Find(Formation.TargetFormationId);
			if (!Target || Target->Members.IsEmpty())
			{
				Formation.OrderType = EMassWarFormationOrderType::Idle; // enemy formation is gone
				Formation.TargetFormationId = 0;
			}
			else
			{
				AssignTargets(EntityManager, Formation, *Target);
			}
		}

		PoolKnowledge(EntityManager, Formation, Now);
		if (Formation.OrderType == EMassWarFormationOrderType::Idle)
		{
			AutoBehave(EntityManager, Formation, Now);
		}
	}

	for (const uint32 Id : Empty)
	{
		Formations.Remove(Id);
	}
}
