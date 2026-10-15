// Copyright Epic Games, Inc. All Rights Reserved.

#include "Spawning/MassWarSpawnSubsystem.h"
#include "Spawning/MassWarFormationTemplate.h"
#include "Formation/MassWarFormationSubsystem.h"
#include "Registry/MassWarUnitRegistrySubsystem.h"
#include "Fragments/MassWarUnitFragments.h"
#include "MassCommonFragments.h"
#include "MassEntityConfigAsset.h"
#include "MassEntityManager.h"
#include "MassEntityTemplate.h"
#include "MassSpawnerSubsystem.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassWarSpawnSubsystem)

FMassWarFormationSpawnParams UMassWarFormationTemplate::MakeParams(int32 OwningPlayerId, int32 TeamId, const FVector& Location, float YawDegrees) const
{
	FMassWarFormationSpawnParams Params;
	Params.Units = Units;
	Params.Settings = Settings;
	Params.OwningPlayerId = OwningPlayerId;
	Params.TeamId = TeamId;
	Params.Location = Location;
	Params.YawDegrees = YawDegrees;
	return Params;
}

bool UMassWarSpawnSubsystem::CanSpawn(const TCHAR* Caller) const
{
	const UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
	{
		UE_LOG(LogTemp, Warning, TEXT("MassWarSpawning: %s ignored - units are spawned on the server (or in standalone) only."), Caller);
		return false;
	}
	return true;
}

bool UMassWarSpawnSubsystem::SpawnRoster(const TArray<FMassWarUnitSpawnEntry>& Roster, int32 OwningPlayerId, int32 TeamId, const FVector& Location,
	float YawDegrees, float Spacing, TArray<FMassEntityHandle>& OutUnits) const
{
	UWorld* World = GetWorld();
	UMassSpawnerSubsystem* Spawner = World ? World->GetSubsystem<UMassSpawnerSubsystem>() : nullptr;
	if (!Spawner)
	{
		UE_LOG(LogTemp, Error, TEXT("MassWarSpawning: no UMassSpawnerSubsystem in this world."));
		return false;
	}

	int32 Total = 0;
	for (const FMassWarUnitSpawnEntry& Entry : Roster)
	{
		Total += (Entry.Config && Entry.Count > 0) ? Entry.Count : 0;
	}
	if (Total == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("MassWarSpawning: nothing to spawn (every roster entry has no Config or a Count of 0)."));
		return false;
	}

	FMassEntityManager& EntityManager = Spawner->GetEntityManagerChecked();
	UMassWarUnitRegistrySubsystem* Registry = World->GetSubsystem<UMassWarUnitRegistrySubsystem>();
	const UMassWarWeaponSubsystem* Weapons = World->GetSubsystem<UMassWarWeaponSubsystem>();

	// One grid for the whole roster, centred on Location and turned to face YawDegrees.
	const int32 Columns = FMath::Max(1, FMath::CeilToInt(FMath::Sqrt(static_cast<float>(Total))));
	const int32 Rows = FMath::DivideAndRoundUp(Total, Columns);
	const float Step = FMath::Max(Spacing, 10.f);
	const FRotator Facing(0.f, YawDegrees, 0.f);

	int32 Index = 0;
	for (const FMassWarUnitSpawnEntry& Entry : Roster)
	{
		if (!Entry.Config || Entry.Count <= 0)
		{
			continue;
		}

		const FMassEntityTemplate& Template = Entry.Config->GetOrCreateEntityTemplate(*World);
		TArray<FMassEntityHandle> Spawned;
		Spawner->SpawnEntities(Template, static_cast<uint32>(Entry.Count), Spawned);

		const bool bHasLoadout = Entry.Loadout.Primary || Entry.Loadout.Secondary || Entry.Loadout.Special;
		bool bWarnedTeam = false;
		bool bWarnedWeapons = false;

		for (const FMassEntityHandle& Entity : Spawned)
		{
			if (Registry)
			{
				Registry->RegisterUnit(Entity);
			}

			if (FMassWarTeamFragment* Team = EntityManager.GetFragmentDataPtr<FMassWarTeamFragment>(Entity))
			{
				Team->TeamId = static_cast<uint8>(TeamId);
			}
			else if (!bWarnedTeam)
			{
				bWarnedTeam = true;
				UE_LOG(LogTemp, Warning, TEXT("MassWarSpawning: %s has no team fragment - add the \"MassWar Unit\" trait to it."), *GetNameSafe(Entry.Config));
			}

			if (FMassWarOwnerFragment* Owner = EntityManager.GetFragmentDataPtr<FMassWarOwnerFragment>(Entity))
			{
				Owner->OwningPlayerId = static_cast<uint32>(OwningPlayerId);
			}

			if (FTransformFragment* Transform = EntityManager.GetFragmentDataPtr<FTransformFragment>(Entity))
			{
				const int32 Row = Index / Columns;
				const int32 Column = Index % Columns;
				const FVector Local((Row - (Rows - 1) * 0.5f) * Step, (Column - (Columns - 1) * 0.5f) * Step, 0.f);
				Transform->SetTransform(FTransform(Facing, Location + Facing.RotateVector(Local)));
			}

			if (bHasLoadout && Weapons && !Weapons->ApplyLoadout(EntityManager, Entity, Entry.Loadout) && !bWarnedWeapons)
			{
				bWarnedWeapons = true;
				UE_LOG(LogTemp, Warning, TEXT("MassWarSpawning: %s cannot carry weapons - add the \"MassWar Weapons\" trait to it."), *GetNameSafe(Entry.Config));
			}

			++Index;
		}
		OutUnits.Append(Spawned);
	}
	return true;
}

FMassWarSpawnedFormation UMassWarSpawnSubsystem::SpawnFormationWithUnits(const FMassWarFormationSpawnParams& Params, TArray<FMassEntityHandle>& OutUnits)
{
	FMassWarSpawnedFormation Result;
	UWorld* World = GetWorld();
	if (!CanSpawn(TEXT("SpawnFormation")))
	{
		return Result;
	}
	UMassWarFormationSubsystem* Formations = World->GetSubsystem<UMassWarFormationSubsystem>();
	UMassSpawnerSubsystem* Spawner = World->GetSubsystem<UMassSpawnerSubsystem>();
	if (!Formations || !Spawner)
	{
		UE_LOG(LogTemp, Error, TEXT("MassWarSpawning: SpawnFormation needs the formation and Mass spawner subsystems."));
		return Result;
	}

	OutUnits.Reset();
	if (!SpawnRoster(Params.Units, Params.OwningPlayerId, Params.TeamId, Params.Location, Params.YawDegrees, Params.Settings.Spacing, OutUnits))
	{
		return Result;
	}

	Result.FormationId = static_cast<int32>(Formations->CreateFormation(Spawner->GetEntityManagerChecked(), OutUnits, static_cast<uint32>(Params.OwningPlayerId), Params.Settings));
	Result.NumUnits = OutUnits.Num();
	return Result;
}

FMassWarSpawnedFormation UMassWarSpawnSubsystem::SpawnFormation(const FMassWarFormationSpawnParams& Params)
{
	TArray<FMassEntityHandle> Units;
	return SpawnFormationWithUnits(Params, Units);
}

FMassWarSpawnedFormation UMassWarSpawnSubsystem::SpawnFormationFromTemplate(const UMassWarFormationTemplate* Template, int32 OwningPlayerId, int32 TeamId, FVector Location, float YawDegrees)
{
	if (!Template)
	{
		UE_LOG(LogTemp, Warning, TEXT("MassWarSpawning: SpawnFormationFromTemplate called without a template."));
		return FMassWarSpawnedFormation();
	}
	return SpawnFormation(Template->MakeParams(OwningPlayerId, TeamId, Location, YawDegrees));
}

int32 UMassWarSpawnSubsystem::SpawnUnitsWithEntities(const FMassWarUnitSpawnEntry& Entry, int32 OwningPlayerId, int32 TeamId, const FVector& Location,
	float YawDegrees, float Spacing, TArray<FMassEntityHandle>& OutUnits)
{
	OutUnits.Reset();
	if (!CanSpawn(TEXT("SpawnUnits")))
	{
		return 0;
	}
	SpawnRoster({ Entry }, OwningPlayerId, TeamId, Location, YawDegrees, Spacing, OutUnits);
	return OutUnits.Num();
}

int32 UMassWarSpawnSubsystem::SpawnUnits(const FMassWarUnitSpawnEntry& Entry, int32 OwningPlayerId, int32 TeamId, FVector Location, float YawDegrees, float Spacing)
{
	TArray<FMassEntityHandle> Units;
	return SpawnUnitsWithEntities(Entry, OwningPlayerId, TeamId, Location, YawDegrees, Spacing, Units);
}
