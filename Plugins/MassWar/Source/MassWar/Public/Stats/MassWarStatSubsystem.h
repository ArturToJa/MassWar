// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "HAL/ThreadSafeBool.h"
#include "MassEntityHandle.h"
#include "Stats/MassWarStatTypes.h"
#include "MassWarStatSubsystem.generated.h"

class UMassWarStatSet;
struct FMassEntityManager;
struct FMassEntityTemplateBuildContext;

/**
 * Where unit numbers live. A "stat" is a named float (a built-in such as AttackRange, or one the project defines in
 * Project Settings > MassWar Stats); every unit has a stat PROFILE - one row of resolved values - and the systems that
 * need a number read it from there instead of from fields baked into fragments. That is what lets weapons, ammo and
 * upgrades change a number without any system knowing about them.
 *
 * Build time: the traits of a unit type (MassWar Unit, MassWar Combat, MassWar Stats...) each contribute base values and
 * modifiers to the type's profile - in any order, they find the shared profile through ContributeBase / GetOrCreateProfile.
 * Run time: profiles are resolved lazily and cached; reading a value is one array lookup (GetView() once per processor
 * run, then FView::Get per unit). Units built from the same type share one profile, so a unit costs two bytes.
 */
UCLASS()
class MASSWAR_API UMassWarStatSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	//~ Registry

	/** The stat with this tag (invalid if the tag is not a built-in or a project stat). */
	FMassWarStatId FindStat(const FGameplayTag& Tag) const;
	int32 GetNumStats() const { return Defaults.Num(); }
	FGameplayTag GetStatTag(FMassWarStatId Stat) const { return Tags.IsValidIndex(Stat.Index) ? Tags[Stat.Index] : FGameplayTag(); }
	float GetDefaultValue(FMassWarStatId Stat) const { return Defaults.IsValidIndex(Stat.Index) ? Defaults[Stat.Index] : 0.f; }

	//~ Build time (called by traits while a unit type's template is built)

	/** The stat profile of the unit type being built, created by whichever trait asks first. */
	static uint16 GetOrCreateProfile(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World);

	/** Sets the base value of a built-in stat for the unit type being built (what MassWar Unit and MassWar Combat do with their properties). */
	static void ContributeBase(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World, FMassWarStatId Stat, float Value);

	/** Adds a modifier / a whole stat set to the unit type being built. */
	static void ContributeModifier(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World, const FMassWarStatModifier& Modifier);
	static void ContributeStatSet(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World, const UMassWarStatSet* StatSet);

	//~ Run time

	/** Read access to every resolved profile, valid for the duration of one processor run. */
	struct FView
	{
		const float* Data = nullptr;
		int32 Stride = 0;

		float Get(uint16 Profile, FMassWarStatId Stat) const { return Data[static_cast<int32>(Profile) * Stride + Stat.Index]; }
	};
	FView GetView() const;

	/** A unit's value of a stat (the stat's default if it has no profile). Slower than FView - for code that is not a per-unit loop. */
	float GetValue(const FMassEntityManager& EntityManager, FMassEntityHandle Entity, FMassWarStatId Stat) const;
	float GetValue(const FMassEntityManager& EntityManager, FMassEntityHandle Entity, const FGameplayTag& StatTag) const;

	//~ Run-time stat sets (upgrades, buffs...). The server decides; the ids replicate and clients derive the same profile.

	/** The stat set behind a catalog id (UMassWarStatSettings::StatSetCatalog; null for 0 / unknown). */
	const UMassWarStatSet* GetStatSet(uint8 Id) const { return StatSetsById.IsValidIndex(Id) ? StatSetsById[Id].Get() : nullptr; }

	/** The catalog id of a stat set, 0 if it is null or not in the catalog (only catalogued sets can be applied, because only ids replicate). */
	uint8 FindStatSetId(const UMassWarStatSet* StatSet) const;

	/** Replaces the catalog from code instead of the project setting (tests, mods). Id = position + 1. */
	void SetCatalogOverride(const TArray<UMassWarStatSet*>& StatSets);

	/** Gives a unit exactly these stat sets (at most MassWarMaxRuntimeStatSets; 0 = empty slot). Server / standalone; on clients the ids arrive through replication. */
	bool SetEntityStatSets(FMassEntityManager& EntityManager, FMassEntityHandle Entity, TConstArrayView<uint8> Ids);

	/** Adds / removes one catalogued stat set on a unit. False if the set is not in the catalog, the unit has no stat fragments, or all slots are used. */
	bool AddEntityStatSet(FMassEntityManager& EntityManager, FMassEntityHandle Entity, const UMassWarStatSet* StatSet);
	bool RemoveEntityStatSet(FMassEntityManager& EntityManager, FMassEntityHandle Entity, const UMassWarStatSet* StatSet);

	/** The shared implementation, also used by replication on clients: stores the ids and switches the unit to the matching derived profile. */
	void ApplyStatSets(FMassWarStatProfileFragment& Profile, FMassWarStatSetsFragment& StatSets, TConstArrayView<uint8> Ids);

	/** Logs every stat and every profile (MassWar.Stats.Dump). */
	void DumpToLog() const;

	/** Hard cap on profiles (unit types plus derived recipes); their storage is reserved up front. */
	static constexpr int32 MaxProfiles = 1024;

private:
	struct FStatModifier
	{
		FMassWarStatId Stat;
		EMassWarStatOp Op = EMassWarStatOp::Add;
		float Value = 0.f;
	};

	/** What traits contributed to one profile, until it is resolved into Flat. */
	struct FProfileDraft
	{
		TArray<float> Base;
		TBitArray<> BaseIsSet;
		TArray<FStatModifier> Modifiers;
		/** For a profile derived from run-time stat sets: the unit type profile it was derived from. */
		uint16 Parent = FMassWarStatProfileFragment::InvalidProfile;
		bool bDirty = true;
	};

	uint16 CreateDraft();
	uint16 DeriveProfile(uint16 Profile, TConstArrayView<uint8> Ids);
	void LoadCatalog(const TArray<TSoftObjectPtr<UMassWarStatSet>>& Soft);
	void SetBase(uint16 Profile, FMassWarStatId Stat, float Value);
	void AddModifier(uint16 Profile, const FMassWarStatModifier& Modifier);
	void ResolveDirty() const;

	TArray<FGameplayTag> Tags;
	TArray<float> Defaults;
	TMap<FGameplayTag, uint8> TagToStat;
	int32 Stride = 0;

	/** Catalog id -> stat set (index 0 unused). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<const UMassWarStatSet>> StatSetsById;

	/** (unit type profile, stat set ids) -> the derived profile, so units with the same recipe share one. */
	TMap<uint64, uint16> DerivedProfiles;

	mutable FCriticalSection ResolveLock;
	mutable FThreadSafeBool bAnyDirty;
	mutable TArray<FProfileDraft> Drafts;
	/** Resolved values, Stride floats per profile. Space for MaxProfiles is reserved up front so the pointer handed out by GetView stays valid. */
	mutable TArray<float> Flat;
};
