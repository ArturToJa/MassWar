// Copyright Epic Games, Inc. All Rights Reserved.

#include "Processors/MassWarAmmoProcessor.h"
#include "Processors/MassWarDamageProcessor.h"
#include "Fragments/MassWarUnitFragments.h"
#include "Fragments/MassWarAmmoUtil.h"
#include "Stats/MassWarStatSubsystem.h"
#include "MassExecutionContext.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassWarAmmoProcessor)

UMassWarAmmoProcessor::UMassWarAmmoProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
	ExecutionOrder.ExecuteBefore.Add(UMassWarDamageProcessor::StaticClass()->GetFName());
}

void UMassWarAmmoProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassWarAmmoFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassWarLoadoutFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassWarStatProfileFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassWarLifeFragment>(EMassFragmentAccess::ReadOnly);
}

void UMassWarAmmoProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const UWorld* World = EntityManager.GetWorld();
	const UMassWarStatSubsystem* StatSubsystem = World ? World->GetSubsystem<UMassWarStatSubsystem>() : nullptr;
	if (!StatSubsystem)
	{
		return;
	}
	const UMassWarStatSubsystem::FView Stats = StatSubsystem->GetView();
	const float DeltaTime = Context.GetDeltaTimeSeconds();

	EntityQuery.ForEachEntityChunk(Context, [Stats, DeltaTime](FMassExecutionContext& Context)
	{
		const TArrayView<FMassWarAmmoFragment> AmmoList = Context.GetMutableFragmentView<FMassWarAmmoFragment>();
		const TConstArrayView<FMassWarLoadoutFragment> LoadoutList = Context.GetFragmentView<FMassWarLoadoutFragment>();
		const TConstArrayView<FMassWarStatProfileFragment> ProfileList = Context.GetFragmentView<FMassWarStatProfileFragment>();
		const TConstArrayView<FMassWarLifeFragment> LifeList = Context.GetFragmentView<FMassWarLifeFragment>();

		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			FMassWarAmmoFragment& Ammo = AmmoList[It];
			const uint8 Slot = LoadoutList[It].ActiveSlot;
			const uint16 Profile = ProfileList[It].ProfileIndex;
			const int32 Magazine = FMath::RoundToInt(Stats.Get(Profile, EMassWarStat::MagazineSize));

			// Nothing to track: no weapon in hand, no magazine, or a dying unit.
			if (Slot >= MassWarWeaponSlotCount || Magazine <= 0 || LifeList[It].IsDying())
			{
				Ammo.ReloadingSlot = FMassWarAmmoFragment::NotReloading;
				Ammo.ReloadRemaining = 0.f;
				continue;
			}

			// Another weapon was drawn: its reload does not carry over.
			if (Ammo.IsReloading() && Ammo.ReloadingSlot != Slot)
			{
				Ammo.ReloadingSlot = FMassWarAmmoFragment::NotReloading;
				Ammo.ReloadRemaining = 0.f;
			}

			const float ReserveStat = Stats.Get(Profile, EMassWarStat::ReserveAmmo);
			if (Ammo.Loaded[Slot] == FMassWarAmmoFragment::NotInitialised)
			{
				// First time this weapon is in hand: full magazine, full reserve.
				Ammo.Loaded[Slot] = static_cast<uint16>(FMath::Clamp(Magazine, 0, 0xFFFE));
				Ammo.Reserve[Slot] = ReserveStat < 0.f ? 0 : static_cast<uint16>(FMath::Clamp(FMath::RoundToInt(ReserveStat), 0, 0xFFFE));
			}
			else if (Ammo.Loaded[Slot] > Magazine)
			{
				Ammo.Loaded[Slot] = static_cast<uint16>(Magazine); // the magazine got smaller
			}

			if (Ammo.IsReloading())
			{
				Ammo.ReloadRemaining -= DeltaTime;
				if (Ammo.ReloadRemaining <= 0.f)
				{
					const int32 Needed = Magazine - Ammo.Loaded[Slot];
					const bool bUnlimited = ReserveStat < 0.f;
					const int32 Taken = bUnlimited ? Needed : FMath::Min<int32>(Needed, Ammo.Reserve[Slot]);
					Ammo.Loaded[Slot] = static_cast<uint16>(Ammo.Loaded[Slot] + Taken);
					if (!bUnlimited)
					{
						Ammo.Reserve[Slot] = static_cast<uint16>(Ammo.Reserve[Slot] - Taken);
					}
					Ammo.ReloadingSlot = FMassWarAmmoFragment::NotReloading;
					Ammo.ReloadRemaining = 0.f;
				}
			}
		}
	});
}