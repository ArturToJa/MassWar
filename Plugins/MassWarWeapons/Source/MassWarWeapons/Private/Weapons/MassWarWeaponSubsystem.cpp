// Copyright Epic Games, Inc. All Rights Reserved.

#include "Weapons/MassWarWeaponSubsystem.h"
#include "Weapons/MassWarWeaponDefinition.h"
#include "Weapons/MassWarWeaponCatalog.h"
#include "Weapons/MassWarAmmoDefinition.h"
#include "Stats/MassWarStatSubsystem.h"
#include "Fragments/MassWarAmmoUtil.h"
#include "Stats/MassWarStatSet.h"
#include "MassEntityView.h"
#include "Engine/World.h"
#include "MassEntityManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassWarWeaponSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogMassWarWeapons, Log, All);

UMassWarWeaponDefinition* FMassWarLoadout::Get(EMassWarWeaponSlot Slot) const
{
	switch (Slot)
	{
	case EMassWarWeaponSlot::Primary: return Primary;
	case EMassWarWeaponSlot::Secondary: return Secondary;
	case EMassWarWeaponSlot::Special: return Special;
	}
	return nullptr;
}

UMassWarAmmoDefinition* FMassWarLoadout::GetAmmo(EMassWarWeaponSlot Slot) const
{
	switch (Slot)
	{
	case EMassWarWeaponSlot::Primary: return PrimaryAmmo;
	case EMassWarWeaponSlot::Secondary: return SecondaryAmmo;
	case EMassWarWeaponSlot::Special: return SpecialAmmo;
	}
	return nullptr;
}

void UMassWarWeaponSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<UMassWarStatSubsystem>();
	Super::Initialize(Collection);

	if (const UMassWarWeaponSettings* Settings = GetDefault<UMassWarWeaponSettings>())
	{
		Catalog = Settings->Catalog.IsNull() ? nullptr : Settings->Catalog.LoadSynchronous();
		AmmoCatalog = Settings->AmmoCatalog.IsNull() ? nullptr : Settings->AmmoCatalog.LoadSynchronous();
	}
	if (!Catalog)
	{
		UE_LOG(LogMassWarWeapons, Log, TEXT("No weapon catalog set (Project Settings > Plugins > MassWar Weapons): units will carry no weapons."));
	}
	RebuildLookup();

	// A unit's stats include those of the weapon in its hand and of the ammo loaded in it.
	if (UMassWarStatSubsystem* Stats = GetWorld() ? GetWorld()->GetSubsystem<UMassWarStatSubsystem>() : nullptr)
	{
		Stats->RegisterSourceCollector(TEXT("MassWarWeapons"), [this](const FMassEntityView& Unit, TArray<const UMassWarStatSet*>& OutStatSets)
		{
			const FMassWarLoadoutFragment* Loadout = Unit.GetFragmentDataPtr<FMassWarLoadoutFragment>();
			if (!Loadout)
			{
				return;
			}
			if (const UMassWarWeaponDefinition* Weapon = GetDefinition(Loadout->GetActiveWeaponId()))
			{
				OutStatSets.Add(Weapon->StatSet);
			}
			if (const UMassWarAmmoDefinition* Ammo = GetAmmo(Loadout->GetActiveAmmoId()))
			{
				OutStatSets.Add(Ammo->StatSet);
			}
		});
	}
}

void UMassWarWeaponSubsystem::Deinitialize()
{
	if (UMassWarStatSubsystem* Stats = GetWorld() ? GetWorld()->GetSubsystem<UMassWarStatSubsystem>() : nullptr)
	{
		Stats->UnregisterSourceCollector(TEXT("MassWarWeapons"));
	}
	Super::Deinitialize();
}

void UMassWarWeaponSubsystem::SetCatalogOverride(UMassWarWeaponCatalog* InCatalog)
{
	Catalog = InCatalog;
	RebuildLookup();
}

void UMassWarWeaponSubsystem::SetAmmoCatalogOverride(UMassWarAmmoCatalog* InCatalog)
{
	AmmoCatalog = InCatalog;
	RebuildLookup();
}

const UMassWarAmmoDefinition* UMassWarWeaponSubsystem::GetAmmo(const uint8 AmmoId) const
{
	return (AmmoId > 0 && AmmoById.IsValidIndex(AmmoId - 1)) ? AmmoById[AmmoId - 1].Get() : nullptr;
}

uint8 UMassWarWeaponSubsystem::FindAmmoId(const UMassWarAmmoDefinition* Ammo) const
{
	if (!Ammo)
	{
		return 0;
	}
	const int32 Index = AmmoById.IndexOfByKey(Ammo);
	if (Index == INDEX_NONE)
	{
		UE_LOG(LogMassWarWeapons, Warning, TEXT("Ammo %s is not in the ammo catalog - the unit will not use it."), *GetNameSafe(Ammo));
		return 0;
	}
	return static_cast<uint8>(Index + 1);
}

void UMassWarWeaponSubsystem::RebuildLookup()
{
	AmmoById.Reset();
	if (AmmoCatalog)
	{
		const int32 NumAmmo = FMath::Min(AmmoCatalog->Ammo.Num(), 255);
		for (int32 Index = 0; Index < NumAmmo; ++Index)
		{
			AmmoById.Add(AmmoCatalog->Ammo[Index]);
		}
	}

	ById.Reset();
	if (!Catalog)
	{
		return;
	}
	ensureMsgf(Catalog->Weapons.Num() <= 255, TEXT("A weapon catalog holds at most 255 weapons; the rest are ignored."));
	const int32 Num = FMath::Min(Catalog->Weapons.Num(), 255);
	ById.Reserve(Num);
	for (int32 Index = 0; Index < Num; ++Index)
	{
		ById.Add(Catalog->Weapons[Index]);
	}
}

const UMassWarWeaponDefinition* UMassWarWeaponSubsystem::GetDefinition(const uint8 WeaponId) const
{
	return (WeaponId > 0 && ById.IsValidIndex(WeaponId - 1)) ? ById[WeaponId - 1].Get() : nullptr;
}

uint8 UMassWarWeaponSubsystem::FindId(const UMassWarWeaponDefinition* Definition) const
{
	if (!Definition)
	{
		return 0;
	}
	const int32 Index = ById.IndexOfByKey(Definition);
	if (Index == INDEX_NONE)
	{
		UE_LOG(LogMassWarWeapons, Warning, TEXT("Weapon %s is not in the weapon catalog - the unit will not carry it."), *GetNameSafe(Definition));
		return 0;
	}
	return static_cast<uint8>(Index + 1);
}

FMassWarLoadoutFragment UMassWarWeaponSubsystem::MakeFragment(const FMassWarLoadout& Loadout) const
{
	FMassWarLoadoutFragment Fragment;
	for (int32 SlotIndex = 0; SlotIndex < MassWarWeaponSlotCount; ++SlotIndex)
	{
		const EMassWarWeaponSlot Slot = static_cast<EMassWarWeaponSlot>(SlotIndex);
		const UMassWarWeaponDefinition* Weapon = Loadout.Get(Slot);
		if (Weapon && Weapon->Slot != Slot)
		{
			UE_LOG(LogMassWarWeapons, Warning, TEXT("Weapon %s is a %s weapon but was placed in the %s slot."), *GetNameSafe(Weapon),
				*UEnum::GetValueAsString(Weapon->Slot), *UEnum::GetValueAsString(Slot));
		}
		Fragment.WeaponIds[SlotIndex] = FindId(Weapon);

		// The requested ammo if the weapon accepts it, else the weapon's default.
		const UMassWarAmmoDefinition* Ammo = Loadout.GetAmmo(Slot);
		if (Weapon && Ammo && Weapon->AllowedAmmo.Num() > 0 && !Weapon->AllowedAmmo.Contains(Ammo))
		{
			UE_LOG(LogMassWarWeapons, Warning, TEXT("Weapon %s does not accept ammo %s - using its default ammo."), *GetNameSafe(Weapon), *GetNameSafe(Ammo));
			Ammo = nullptr;
		}
		Fragment.AmmoIds[SlotIndex] = FindAmmoId(Weapon ? (Ammo ? Ammo : Weapon->DefaultAmmo.Get()) : nullptr);
	}
	// In hand: the first slot that holds something.
	for (int32 SlotIndex = 0; SlotIndex < MassWarWeaponSlotCount; ++SlotIndex)
	{
		if (Fragment.WeaponIds[SlotIndex] != 0)
		{
			Fragment.ActiveSlot = static_cast<uint8>(SlotIndex);
			break;
		}
	}
	return Fragment;
}

bool UMassWarWeaponSubsystem::ApplyLoadout(FMassEntityManager& EntityManager, FMassEntityHandle Entity, const FMassWarLoadout& Loadout) const
{
	FMassWarLoadoutFragment* Fragment = EntityManager.IsEntityValid(Entity) ? EntityManager.GetFragmentDataPtr<FMassWarLoadoutFragment>(Entity) : nullptr;
	if (!Fragment)
	{
		return false;
	}
	*Fragment = MakeFragment(Loadout);
	RefreshStats(EntityManager, Entity);
	return true;
}

bool UMassWarWeaponSubsystem::SetActiveSlot(FMassEntityManager& EntityManager, FMassEntityHandle Entity, EMassWarWeaponSlot Slot) const
{
	FMassWarLoadoutFragment* Fragment = EntityManager.IsEntityValid(Entity) ? EntityManager.GetFragmentDataPtr<FMassWarLoadoutFragment>(Entity) : nullptr;
	const int32 SlotIndex = static_cast<int32>(Slot);
	if (!Fragment || SlotIndex >= MassWarWeaponSlotCount || Fragment->WeaponIds[SlotIndex] == 0)
	{
		return false;
	}
	Fragment->ActiveSlot = static_cast<uint8>(SlotIndex);
	RefreshStats(EntityManager, Entity);
	return true;
}

bool UMassWarWeaponSubsystem::SetAmmo(FMassEntityManager& EntityManager, FMassEntityHandle Entity, EMassWarWeaponSlot Slot, const UMassWarAmmoDefinition* Ammo) const
{
	FMassWarLoadoutFragment* Fragment = EntityManager.IsEntityValid(Entity) ? EntityManager.GetFragmentDataPtr<FMassWarLoadoutFragment>(Entity) : nullptr;
	const int32 SlotIndex = static_cast<int32>(Slot);
	if (!Fragment || SlotIndex >= MassWarWeaponSlotCount)
	{
		return false;
	}
	const UMassWarWeaponDefinition* Weapon = GetDefinition(Fragment->WeaponIds[SlotIndex]);
	if (!Weapon || (Ammo && Weapon->AllowedAmmo.Num() > 0 && !Weapon->AllowedAmmo.Contains(Ammo)))
	{
		return false;
	}
	Fragment->AmmoIds[SlotIndex] = FindAmmoId(Ammo);
	// New ammo goes into a fresh magazine: that slot's ammunition is set up again the next time it is looked at.
	if (FMassWarAmmoFragment* AmmoState = EntityManager.GetFragmentDataPtr<FMassWarAmmoFragment>(Entity))
	{
		AmmoState->Loaded[SlotIndex] = FMassWarAmmoFragment::NotInitialised;
		if (AmmoState->ReloadingSlot == SlotIndex)
		{
			AmmoState->ReloadingSlot = FMassWarAmmoFragment::NotReloading;
			AmmoState->ReloadRemaining = 0.f;
		}
	}
	RefreshStats(EntityManager, Entity);
	return true;
}

void UMassWarWeaponSubsystem::RefreshStats(FMassEntityManager& EntityManager, FMassEntityHandle Entity) const
{
	if (UMassWarStatSubsystem* Stats = GetWorld() ? GetWorld()->GetSubsystem<UMassWarStatSubsystem>() : nullptr)
	{
		Stats->RefreshProfile(FMassEntityView(EntityManager, Entity));
	}
}

bool UMassWarWeaponSubsystem::Reload(FMassEntityManager& EntityManager, FMassEntityHandle Entity) const
{
	if (!EntityManager.IsEntityValid(Entity))
	{
		return false;
	}
	FMassWarAmmoFragment* Ammo = EntityManager.GetFragmentDataPtr<FMassWarAmmoFragment>(Entity);
	const FMassWarLoadoutFragment* Loadout = EntityManager.GetFragmentDataPtr<FMassWarLoadoutFragment>(Entity);
	const FMassWarStatProfileFragment* Profile = EntityManager.GetFragmentDataPtr<FMassWarStatProfileFragment>(Entity);
	const UMassWarStatSubsystem* Stats = GetWorld() ? GetWorld()->GetSubsystem<UMassWarStatSubsystem>() : nullptr;
	if (!Ammo || !Loadout || !Profile || !Stats)
	{
		return false;
	}
	return MassWarAmmo::TryStartReload(*Ammo, Loadout->ActiveSlot, Stats->GetView(), Profile->ProfileIndex);
}