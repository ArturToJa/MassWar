// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Fragments/MassWarUnitFragments.h"
#include "Stats/MassWarStatSubsystem.h"

namespace MassWarAmmo
{
	/**
	 * Starts reloading the weapon in Slot, if that makes sense: the weapon has a magazine (MagazineSize stat above 0), it is not full,
	 * no reload is running, and there is reserve ammo to load (or the reserve is unlimited). The reload takes the ReloadTime stat.
	 * Used by combat when a magazine runs dry and by Weapons for a reload on command. Returns true if a reload started.
	 */
	inline bool TryStartReload(FMassWarAmmoFragment& Ammo, uint8 Slot, const UMassWarStatSubsystem::FView& Stats, uint16 Profile)
	{
		if (Slot >= MassWarWeaponSlotCount || Ammo.IsReloading() || Ammo.Loaded[Slot] == FMassWarAmmoFragment::NotInitialised)
		{
			return false;
		}
		const int32 Magazine = FMath::RoundToInt(Stats.Get(Profile, EMassWarStat::MagazineSize));
		if (Magazine <= 0 || Ammo.Loaded[Slot] >= Magazine)
		{
			return false;
		}
		const bool bUnlimitedReserve = Stats.Get(Profile, EMassWarStat::ReserveAmmo) < 0.f;
		if (!bUnlimitedReserve && Ammo.Reserve[Slot] == 0)
		{
			return false;
		}
		Ammo.ReloadingSlot = Slot;
		Ammo.ReloadRemaining = FMath::Max(Stats.Get(Profile, EMassWarStat::ReloadTime), 0.05f);
		return true;
	}
}