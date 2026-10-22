// Copyright Epic Games, Inc. All Rights Reserved.

#include "Stats/MassWarStatSubsystem.h"
#include "Stats/MassWarStatSettings.h"
#include "Stats/MassWarStatSet.h"
#include "MassEntityManager.h"
#include "MassEntityView.h"
#include "MassEntityTemplateRegistry.h"
#include "NativeGameplayTags.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassWarStatSubsystem)

namespace MassWarStatTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(MoveSpeed, "MassWar.Stat.MoveSpeed", "Units per second a unit walks at.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TurnRate, "MassWar.Stat.TurnRate", "Degrees per second a unit turns (yaw).")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AcceptanceRadius, "MassWar.Stat.AcceptanceRadius", "Move orders: how close to the destination counts as arrived.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AttackStopDistance, "MassWar.Stat.AttackStopDistance", "Attack orders: distance from the target at which the unit stops closing in.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AvoidanceRadius, "MassWar.Stat.AvoidanceRadius", "Personal-space radius used by unit-vs-unit avoidance.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AvoidanceStrength, "MassWar.Stat.AvoidanceStrength", "Avoidance push strength as a fraction of move speed.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(DeathLingerTime, "MassWar.Stat.DeathLingerTime", "Seconds a dead unit lingers before it is removed.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(MaxHealth, "MassWar.Stat.MaxHealth", "Maximum health.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AttackDamage, "MassWar.Stat.AttackDamage", "Damage per landed attack.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AttackRange, "MassWar.Stat.AttackRange", "Distance within which a unit can attack its target.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AttackInterval, "MassWar.Stat.AttackInterval", "Seconds between attacks.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AttackNoiseRange, "MassWar.Stat.AttackNoiseRange", "How far enemies can hear this unit's attacks.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(MagazineSize, "MassWar.Stat.MagazineSize", "Rounds the weapon holds before it must reload. 0 = no ammo limit, never reloads.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(ReserveAmmo, "MassWar.Stat.ReserveAmmo", "Rounds carried in reserve for reloading. Negative = unlimited.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(ReloadTime, "MassWar.Stat.ReloadTime", "Seconds a reload takes.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AmmoPerAttack, "MassWar.Stat.AmmoPerAttack", "Rounds one attack uses.")
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AutoReload, "MassWar.Stat.AutoReload", "1 = the unit reloads by itself when the magazine runs empty, 0 = only on command.")

	struct FBuiltIn
	{
		const FNativeGameplayTag* Tag;
		float Default;
	};

	/** In EMassWarStat order. The defaults equal the defaults of the MassWar Unit / MassWar Combat trait properties. */
	const FBuiltIn& GetBuiltIn(EMassWarStat Stat)
	{
		static const FBuiltIn BuiltIns[] = {
			{ &MoveSpeed, 500.f },
			{ &TurnRate, 720.f },
			{ &AcceptanceRadius, 50.f },
			{ &AttackStopDistance, 100.f },
			{ &AvoidanceRadius, 40.f },
			{ &AvoidanceStrength, 1.f },
			{ &DeathLingerTime, 3.f },
			{ &MaxHealth, 100.f },
			{ &AttackDamage, 10.f },
			{ &AttackRange, 800.f },
			{ &AttackInterval, 1.f },
			{ &AttackNoiseRange, 2500.f },
			{ &MagazineSize, 0.f },
			{ &ReserveAmmo, -1.f },
			{ &ReloadTime, 2.f },
			{ &AmmoPerAttack, 1.f },
			{ &AutoReload, 1.f },
		};
		static_assert(UE_ARRAY_COUNT(BuiltIns) == static_cast<int32>(EMassWarStat::Count), "one entry per EMassWarStat");
		return BuiltIns[static_cast<int32>(Stat)];
	}
}

//~ Reader, calculator, library

float FMassWarStatReader::Get(FMassWarStatId Stat) const
{
	return (IsValid() && Stat.IsValid() && Stat.Index < Subsystem->GetNumStats()) ? Values[Stat.Index] : 0.f;
}

float FMassWarStatReader::Get(const FGameplayTag& StatTag) const
{
	return IsValid() ? Get(Subsystem->FindStat(StatTag)) : 0.f;
}

float UMassWarStatCalculator::Calculate_Implementation(const FMassWarStatReader& Stats) const
{
	return Stats.Get(OutputStat);
}

float UMassWarStatLibrary::GetStat(const FMassWarStatReader& Stats, FGameplayTag StatTag)
{
	return Stats.Get(StatTag);
}

float UMassWarStatLibrary::GetBuiltInStat(const FMassWarStatReader& Stats, EMassWarStat Stat)
{
	return Stats.Get(FMassWarStatId(Stat));
}

//~ Registry

void UMassWarStatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Tags.Reset();
	Defaults.Reset();
	TagToStat.Reset();

	for (int32 Index = 0; Index < static_cast<int32>(EMassWarStat::Count); ++Index)
	{
		const MassWarStatTags::FBuiltIn& BuiltIn = MassWarStatTags::GetBuiltIn(static_cast<EMassWarStat>(Index));
		Tags.Add(BuiltIn.Tag->GetTag());
		Defaults.Add(BuiltIn.Default);
	}

	const UMassWarStatSettings* Settings = GetDefault<UMassWarStatSettings>();
	if (Settings)
	{
		for (const FMassWarStatDefinition& Definition : Settings->CustomStats)
		{
			if (Tags.Num() >= MassWarMaxStats)
			{
				UE_LOG(LogTemp, Error, TEXT("MassWarStats: more than %d stats defined - the rest are ignored."), MassWarMaxStats);
				break;
			}
			// An empty entry still takes its place so the ids of the stats after it do not shift.
			Tags.Add(Definition.Tag);
			Defaults.Add(Definition.DefaultValue);
		}
	}

	for (int32 Index = 0; Index < Tags.Num(); ++Index)
	{
		if (Tags[Index].IsValid())
		{
			if (TagToStat.Contains(Tags[Index]))
			{
				UE_LOG(LogTemp, Warning, TEXT("MassWarStats: stat %s is defined twice - the first one is used."), *Tags[Index].ToString());
				continue;
			}
			TagToStat.Add(Tags[Index], static_cast<uint8>(Index));
		}
	}

	Stride = Tags.Num();
	Flat.Reserve(MaxProfiles * Stride);

	Calculators.Reset();
	CalculatorOutputs.Reset();
	if (Settings)
	{
		LoadCatalog(Settings->StatSetCatalog);
		for (const TSubclassOf<UMassWarStatCalculator>& CalculatorClass : Settings->StatCalculators)
		{
			if (!CalculatorClass)
			{
				continue;
			}
			UMassWarStatCalculator* Calculator = NewObject<UMassWarStatCalculator>(this, CalculatorClass);
			const FMassWarStatId Output = FindStat(Calculator->OutputStat);
			if (!Output.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("MassWarStats: calculator %s ignored - its Output Stat '%s' is not a built-in or project stat."), *GetNameSafe(CalculatorClass), *Calculator->OutputStat.ToString());
				continue;
			}
			Calculators.Add(Calculator);
			CalculatorOutputs.Add(Output);
		}
	}
}

FMassWarStatId UMassWarStatSubsystem::FindStat(const FGameplayTag& Tag) const
{
	const uint8* Found = Tag.IsValid() ? TagToStat.Find(Tag) : nullptr;
	return Found ? FMassWarStatId(*Found) : FMassWarStatId();
}

//~ Build time

uint16 UMassWarStatSubsystem::GetOrCreateProfile(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World)
{
	if (const FMassWarStatProfileFragment* Existing = BuildContext.GetFragment<FMassWarStatProfileFragment>())
	{
		return Existing->ProfileIndex;
	}

	UMassWarStatSubsystem* Stats = World.GetSubsystem<UMassWarStatSubsystem>();
	if (!ensureMsgf(Stats, TEXT("MassWarStats: no stat subsystem in this world - stats cannot be set up.")))
	{
		return FMassWarStatProfileFragment::InvalidProfile;
	}

	const uint16 Profile = Stats->CreateDraft();
	if (Profile != FMassWarStatProfileFragment::InvalidProfile)
	{
		BuildContext.AddFragment_GetRef<FMassWarStatProfileFragment>().ProfileIndex = Profile;
	}
	return Profile;
}

void UMassWarStatSubsystem::ContributeBase(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World, FMassWarStatId Stat, float Value)
{
	const uint16 Profile = GetOrCreateProfile(BuildContext, World);
	if (Profile != FMassWarStatProfileFragment::InvalidProfile)
	{
		World.GetSubsystem<UMassWarStatSubsystem>()->SetBase(Profile, Stat, Value);
	}
}

void UMassWarStatSubsystem::ContributeModifier(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World, const FMassWarStatModifier& Modifier)
{
	const uint16 Profile = GetOrCreateProfile(BuildContext, World);
	if (Profile != FMassWarStatProfileFragment::InvalidProfile)
	{
		World.GetSubsystem<UMassWarStatSubsystem>()->AddModifier(Profile, Modifier);
	}
}

void UMassWarStatSubsystem::ContributeStatSet(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World, const UMassWarStatSet* StatSet)
{
	if (!StatSet)
	{
		return;
	}
	for (const FMassWarStatModifier& Modifier : StatSet->Modifiers)
	{
		ContributeModifier(BuildContext, World, Modifier);
	}
}

//~ Profiles

uint16 UMassWarStatSubsystem::CreateDraft()
{
	check(IsInGameThread());
	if (Drafts.Num() >= MaxProfiles)
	{
		UE_LOG(LogTemp, Error, TEXT("MassWarStats: out of stat profiles (%d) - too many unit types and stat setups."), MaxProfiles);
		return FMassWarStatProfileFragment::InvalidProfile;
	}
	FProfileDraft& Draft = Drafts.AddDefaulted_GetRef();
	Draft.Base.Init(0.f, Stride);
	Draft.BaseIsSet.Init(false, Stride);
	const uint16 Profile = static_cast<uint16>(Drafts.Num() - 1);
	Resolve(Profile);
	return Profile;
}

void UMassWarStatSubsystem::SetBase(uint16 Profile, FMassWarStatId Stat, float Value)
{
	check(IsInGameThread());
	if (Drafts.IsValidIndex(Profile) && Stat.IsValid() && Stat.Index < Stride)
	{
		Drafts[Profile].Base[Stat.Index] = Value;
		Drafts[Profile].BaseIsSet[Stat.Index] = true;
		Resolve(Profile);
	}
}

void UMassWarStatSubsystem::AddModifier(uint16 Profile, const FMassWarStatModifier& Modifier)
{
	check(IsInGameThread());
	const FMassWarStatId Stat = FindStat(Modifier.Stat);
	if (!Stat.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("MassWarStats: modifier for unknown stat '%s' ignored - is it a built-in (MassWar.Stat.*) or defined in Project Settings > MassWar Stats?"), *Modifier.Stat.ToString());
		return;
	}
	if (Drafts.IsValidIndex(Profile))
	{
		FStatModifier& Entry = Drafts[Profile].Modifiers.AddDefaulted_GetRef();
		Entry.Stat = Stat;
		Entry.Op = Modifier.Op;
		Entry.Value = Modifier.Value;
		Resolve(Profile);
	}
}

void UMassWarStatSubsystem::AppendStatSet(FProfileDraft& Draft, const UMassWarStatSet& StatSet) const
{
	for (const FMassWarStatModifier& Modifier : StatSet.Modifiers)
	{
		const FMassWarStatId Stat = FindStat(Modifier.Stat);
		if (Stat.IsValid())
		{
			FStatModifier& Entry = Draft.Modifiers.AddDefaulted_GetRef();
			Entry.Stat = Stat;
			Entry.Op = Modifier.Op;
			Entry.Value = Modifier.Value;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("MassWarStats: stat set %s has a modifier for unknown stat '%s' - ignored."), *StatSet.GetName(), *Modifier.Stat.ToString());
		}
	}
}

void UMassWarStatSubsystem::Resolve(uint16 Profile)
{
	check(IsInGameThread());
	const FProfileDraft& Draft = Drafts[Profile];

	TArray<float, TInlineAllocator<32>> Row;
	Row.SetNumUninitialized(Stride);
	for (int32 Stat = 0; Stat < Stride; ++Stat)
	{
		float Value = Draft.BaseIsSet[Stat] ? Draft.Base[Stat] : Defaults[Stat];
		float Added = 0.f;
		float Multiplied = 1.f;
		for (const FStatModifier& Modifier : Draft.Modifiers)
		{
			if (Modifier.Stat.Index != Stat)
			{
				continue;
			}
			switch (Modifier.Op)
			{
			case EMassWarStatOp::Set: Value = Modifier.Value; break;
			case EMassWarStatOp::Add: Added += Modifier.Value; break;
			case EMassWarStatOp::Multiply: Multiplied *= Modifier.Value; break;
			}
		}
		Row[Stat] = (Value + Added) * Multiplied;
	}

	// The project's calculators see the values as they are at that point and each writes one stat.
	FMassWarStatReader Reader;
	Reader.Subsystem = this;
	Reader.Values = Row.GetData();
	for (int32 Index = 0; Index < Calculators.Num(); ++Index)
	{
		Row[CalculatorOutputs[Index].Index] = Calculators[Index]->Calculate(Reader);
	}

	// Space for MaxProfiles was reserved, so this never moves the storage processors are reading.
	if (Flat.Num() < (Profile + 1) * Stride)
	{
		Flat.SetNumZeroed((Profile + 1) * Stride);
	}
	FMemory::Memcpy(&Flat[Profile * Stride], Row.GetData(), Stride * sizeof(float));
}

uint16 UMassWarStatSubsystem::DeriveProfile(uint16 Profile, TConstArrayView<const UMassWarStatSet*> Sets)
{
	check(IsInGameThread());
	if (!Drafts.IsValidIndex(Profile))
	{
		return Profile;
	}

	// Always derive from the unit type's own profile, so the same setup gives the same profile whatever the unit had before.
	const uint16 Root = Drafts[Profile].Parent != FMassWarStatProfileFragment::InvalidProfile ? Drafts[Profile].Parent : Profile;
	if (Sets.IsEmpty())
	{
		return Root;
	}

	FDerivedKey Key;
	Key.Root = Root;
	Key.Sets = Sets;
	if (const uint16* Existing = DerivedProfiles.Find(Key))
	{
		return *Existing;
	}

	if (Drafts.Num() >= MaxProfiles)
	{
		UE_LOG(LogTemp, Error, TEXT("MassWarStats: out of stat profiles (%d) - a stat setup is ignored."), MaxProfiles);
		return Root;
	}

	FProfileDraft Derived = Drafts[Root]; // by value: adding to Drafts may move its storage
	Derived.Parent = Root;
	for (const UMassWarStatSet* StatSet : Sets)
	{
		AppendStatSet(Derived, *StatSet);
	}
	Drafts.Add(MoveTemp(Derived));
	const uint16 NewProfile = static_cast<uint16>(Drafts.Num() - 1);
	Resolve(NewProfile);
	DerivedProfiles.Add(MoveTemp(Key), NewProfile);
	return NewProfile;
}

//~ Sources

void UMassWarStatSubsystem::RegisterSourceCollector(FName Owner, FSourceCollector Collector)
{
	for (FCollectorEntry& Entry : Collectors)
	{
		if (Entry.Owner == Owner)
		{
			Entry.Collect = MoveTemp(Collector);
			return;
		}
	}
	Collectors.Add({ Owner, MoveTemp(Collector) });
}

void UMassWarStatSubsystem::UnregisterSourceCollector(FName Owner)
{
	Collectors.RemoveAll([Owner](const FCollectorEntry& Entry) { return Entry.Owner == Owner; });
}

void UMassWarStatSubsystem::RefreshProfile(const FMassEntityView& Unit)
{
	FMassWarStatProfileFragment* Profile = Unit.GetFragmentDataPtr<FMassWarStatProfileFragment>();
	if (!Profile || Profile->ProfileIndex == FMassWarStatProfileFragment::InvalidProfile)
	{
		return;
	}

	TArray<const UMassWarStatSet*> Sets;
	for (const FCollectorEntry& Entry : Collectors)
	{
		Entry.Collect(Unit, Sets);
	}
	if (const FMassWarStatSetsFragment* Runtime = Unit.GetFragmentDataPtr<FMassWarStatSetsFragment>())
	{
		for (const uint8 Id : Runtime->Ids)
		{
			if (const UMassWarStatSet* StatSet = GetStatSet(Id))
			{
				Sets.Add(StatSet);
			}
			else if (Id != 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("MassWarStats: stat set id %d is not in the catalog on this machine - ignored. Server and clients must share the same Stat Set Catalog."), Id);
			}
		}
	}
	Sets.RemoveAll([](const UMassWarStatSet* StatSet) { return StatSet == nullptr; });

	Profile->ProfileIndex = DeriveProfile(Profile->ProfileIndex, Sets);
}

void UMassWarStatSubsystem::LoadCatalog(const TArray<TSoftObjectPtr<UMassWarStatSet>>& Soft)
{
	StatSetsById.Reset();
	StatSetsById.Add(nullptr); // id 0 = empty slot
	for (const TSoftObjectPtr<UMassWarStatSet>& Entry : Soft)
	{
		if (StatSetsById.Num() > 255)
		{
			UE_LOG(LogTemp, Error, TEXT("MassWarStats: more than 255 stat sets in the catalog - the rest are ignored."));
			break;
		}
		// A missing asset still takes its place so the ids after it do not shift.
		StatSetsById.Add(Entry.IsNull() ? nullptr : Entry.LoadSynchronous());
	}
}

void UMassWarStatSubsystem::SetCatalogOverride(const TArray<UMassWarStatSet*>& StatSets)
{
	StatSetsById.Reset();
	StatSetsById.Add(nullptr);
	for (UMassWarStatSet* StatSet : StatSets)
	{
		StatSetsById.Add(StatSet);
	}
}

uint8 UMassWarStatSubsystem::FindStatSetId(const UMassWarStatSet* StatSet) const
{
	if (StatSet)
	{
		for (int32 Id = 1; Id < StatSetsById.Num(); ++Id)
		{
			if (StatSetsById[Id] == StatSet)
			{
				return static_cast<uint8>(Id);
			}
		}
	}
	return 0;
}

void UMassWarStatSubsystem::ApplyStatSets(const FMassEntityView& Unit, TConstArrayView<uint8> Ids)
{
	FMassWarStatSetsFragment* StatSets = Unit.GetFragmentDataPtr<FMassWarStatSetsFragment>();
	if (!StatSets)
	{
		return;
	}

	// Keep only known, distinct ids, in order.
	uint8 Clean[MassWarMaxRuntimeStatSets] = { 0, 0, 0, 0 };
	int32 Count = 0;
	for (const uint8 Id : Ids)
	{
		if (Id == 0 || Count >= MassWarMaxRuntimeStatSets)
		{
			continue;
		}
		bool bDuplicate = false;
		for (int32 Slot = 0; Slot < Count; ++Slot)
		{
			bDuplicate |= Clean[Slot] == Id;
		}
		if (!bDuplicate)
		{
			Clean[Count++] = Id;
		}
	}

	if (FMemory::Memcmp(Clean, StatSets->Ids, sizeof(Clean)) == 0)
	{
		return;
	}
	FMemory::Memcpy(StatSets->Ids, Clean, sizeof(Clean));
	RefreshProfile(Unit);
}

bool UMassWarStatSubsystem::SetEntityStatSets(FMassEntityManager& EntityManager, FMassEntityHandle Entity, TConstArrayView<uint8> Ids)
{
	if (!EntityManager.IsEntityValid(Entity) || !EntityManager.GetFragmentDataPtr<FMassWarStatSetsFragment>(Entity) || !EntityManager.GetFragmentDataPtr<FMassWarStatProfileFragment>(Entity))
	{
		return false;
	}
	ApplyStatSets(FMassEntityView(EntityManager, Entity), Ids);
	return true;
}

bool UMassWarStatSubsystem::AddEntityStatSet(FMassEntityManager& EntityManager, FMassEntityHandle Entity, const UMassWarStatSet* StatSet)
{
	const uint8 Id = FindStatSetId(StatSet);
	const FMassWarStatSetsFragment* Current = EntityManager.IsEntityValid(Entity) ? EntityManager.GetFragmentDataPtr<FMassWarStatSetsFragment>(Entity) : nullptr;
	if (Id == 0 || !Current)
	{
		UE_LOG(LogTemp, Warning, TEXT("MassWarStats: cannot add %s - it is not in Project Settings > MassWar Stats > Stat Set Catalog, or the unit has no stat set fragment."), *GetNameSafe(StatSet));
		return false;
	}

	uint8 Ids[MassWarMaxRuntimeStatSets];
	FMemory::Memcpy(Ids, Current->Ids, sizeof(Ids));
	int32 FreeSlot = INDEX_NONE;
	for (int32 Slot = 0; Slot < MassWarMaxRuntimeStatSets; ++Slot)
	{
		if (Ids[Slot] == Id)
		{
			return true; // already has it
		}
		if (Ids[Slot] == 0 && FreeSlot == INDEX_NONE)
		{
			FreeSlot = Slot;
		}
	}
	if (FreeSlot == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("MassWarStats: cannot add %s - the unit already has %d stat sets."), *GetNameSafe(StatSet), MassWarMaxRuntimeStatSets);
		return false;
	}
	Ids[FreeSlot] = Id;
	return SetEntityStatSets(EntityManager, Entity, TConstArrayView<uint8>(Ids, MassWarMaxRuntimeStatSets));
}

bool UMassWarStatSubsystem::RemoveEntityStatSet(FMassEntityManager& EntityManager, FMassEntityHandle Entity, const UMassWarStatSet* StatSet)
{
	const uint8 Id = FindStatSetId(StatSet);
	const FMassWarStatSetsFragment* Current = EntityManager.IsEntityValid(Entity) ? EntityManager.GetFragmentDataPtr<FMassWarStatSetsFragment>(Entity) : nullptr;
	if (Id == 0 || !Current)
	{
		return false;
	}

	uint8 Ids[MassWarMaxRuntimeStatSets];
	int32 Count = 0;
	for (int32 Slot = 0; Slot < MassWarMaxRuntimeStatSets; ++Slot)
	{
		Ids[Slot] = 0;
		if (Current->Ids[Slot] != 0 && Current->Ids[Slot] != Id)
		{
			Ids[Count++] = Current->Ids[Slot];
		}
	}
	return SetEntityStatSets(EntityManager, Entity, TConstArrayView<uint8>(Ids, MassWarMaxRuntimeStatSets));
}

//~ Reading

UMassWarStatSubsystem::FView UMassWarStatSubsystem::GetView() const
{
	FView View;
	View.Data = Flat.GetData();
	View.Stride = Stride;
	return View;
}

FMassWarStatReader UMassWarStatSubsystem::MakeReader(uint16 Profile) const
{
	FMassWarStatReader Reader;
	if (Drafts.IsValidIndex(Profile) && Flat.Num() >= (Profile + 1) * Stride)
	{
		Reader.Subsystem = this;
		Reader.Values = &Flat[Profile * Stride];
	}
	return Reader;
}

float UMassWarStatSubsystem::GetValue(const FMassEntityManager& EntityManager, FMassEntityHandle Entity, FMassWarStatId Stat) const
{
	if (!Stat.IsValid() || Stat.Index >= Stride)
	{
		return 0.f;
	}
	const FMassWarStatProfileFragment* Profile = EntityManager.IsEntityValid(Entity) ? EntityManager.GetFragmentDataPtr<FMassWarStatProfileFragment>(Entity) : nullptr;
	if (!Profile || Profile->ProfileIndex == FMassWarStatProfileFragment::InvalidProfile)
	{
		return GetDefaultValue(Stat);
	}
	return GetView().Get(Profile->ProfileIndex, Stat);
}

float UMassWarStatSubsystem::GetValue(const FMassEntityManager& EntityManager, FMassEntityHandle Entity, const FGameplayTag& StatTag) const
{
	return GetValue(EntityManager, Entity, FindStat(StatTag));
}

void UMassWarStatSubsystem::DumpToLog() const
{
	const FView View = GetView();
	UE_LOG(LogTemp, Display, TEXT("MassWarStats: %d stats, %d profiles, %d calculators"), Stride, Drafts.Num(), Calculators.Num());
	for (int32 Profile = 0; Profile < Drafts.Num(); ++Profile)
	{
		FString Line = FString::Printf(TEXT("  profile %d%s:"), Profile, Drafts[Profile].Parent != FMassWarStatProfileFragment::InvalidProfile ? *FString::Printf(TEXT(" (derived from %d)"), Drafts[Profile].Parent) : TEXT(""));
		for (int32 Stat = 0; Stat < Stride; ++Stat)
		{
			Line += FString::Printf(TEXT(" %s=%g"), Tags[Stat].IsValid() ? *Tags[Stat].ToString() : TEXT("(unnamed)"), View.Get(static_cast<uint16>(Profile), FMassWarStatId(static_cast<uint8>(Stat))));
		}
		UE_LOG(LogTemp, Display, TEXT("%s"), *Line);
	}
}

static FAutoConsoleCommandWithWorld GMassWarStatsDump(
	TEXT("MassWar.Stats.Dump"),
	TEXT("Logs every MassWar stat and the resolved values of every stat profile."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (const UMassWarStatSubsystem* Stats = World ? World->GetSubsystem<UMassWarStatSubsystem>() : nullptr)
		{
			Stats->DumpToLog();
		}
	}));
