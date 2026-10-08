// Copyright Epic Games, Inc. All Rights Reserved.

#include "Weapons/MassWarWeaponSubsystem.h"
#include "Weapons/MassWarWeaponDefinition.h"
#include "Weapons/MassWarWeaponCatalog.h"
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

void UMassWarWeaponSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (const UMassWarWeaponSettings* Settings = GetDefault<UMassWarWeaponSettings>())
	{
		Catalog = Settings->Catalog.IsNull() ? nullptr : Settings->Catalog.LoadSynchronous();
	}
	if (!Catalog)
	{
		UE_LOG(LogMassWarWeapons, Log, TEXT("No weapon catalog set (Project Settings > Plugins > MassWar Weapons): units will carry no weapons."));
	}
	RebuildLookup();
}

void UMassWarWeaponSubsystem::SetCatalogOverride(UMassWarWeaponCatalog* InCatalog)
{
	Catalog = InCatalog;
	RebuildLookup();
}

void UMassWarWeaponSubsystem::RebuildLookup()
{
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
	return true;
}
