// Copyright Epic Games, Inc. All Rights Reserved.

#include "MassWarSetupReplicationCommandlet.h"
#include "MassEntityConfigAsset.h"
#include "MassReplicationTrait.h"
#include "Replication/MassWarClientBubble.h"
#include "Replication/MassWarReplicator.h"
#include "Smoothing/MassWarReplicationSmoothingTrait.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"

int32 UMassWarSetupReplicationCommandlet::Main(const FString& Params)
{
	const FString AssetPath = TEXT("/Game/MassWar/DA_MassWarDemoUnit.DA_MassWarDemoUnit");
	const FString PackagePath = TEXT("/Game/MassWar/DA_MassWarDemoUnit");

	UMassEntityConfigAsset* Config = LoadObject<UMassEntityConfigAsset>(nullptr, *AssetPath);
	if (!Config)
	{
		UE_LOG(LogTemp, Error, TEXT("MassWarSetupReplication: could not load %s"), *AssetPath);
		return 1;
	}

	UMassReplicationTrait* ReplicationTrait = nullptr;
	if (const UMassEntityTraitBase* Existing = Config->FindTrait(UMassReplicationTrait::StaticClass()))
	{
		ReplicationTrait = const_cast<UMassReplicationTrait*>(Cast<UMassReplicationTrait>(Existing));
	}
	else
	{
		ReplicationTrait = Cast<UMassReplicationTrait>(Config->AddTrait(UMassReplicationTrait::StaticClass()));
	}

	if (!ReplicationTrait)
	{
		UE_LOG(LogTemp, Error, TEXT("MassWarSetupReplication: failed to add/find UMassReplicationTrait on %s"), *AssetPath);
		return 1;
	}

	ReplicationTrait->Params.BubbleInfoClass = AMassWarClientBubbleInfo::StaticClass();
	ReplicationTrait->Params.ReplicatorClass = UMassWarReplicator::StaticClass();

	// Default Off-LOD distance (5000cm) is too tight for this demo's 4000cm team-origin offset plus a
	// 1500cm spawn scatter, viewed from a bird's-eye camera above the whole battlefield - widen so every
	// spawned unit stays replicated to a connected client regardless of camera position.
	ReplicationTrait->Params.LODDistance[EMassLOD::High] = 2000.f;
	ReplicationTrait->Params.LODDistance[EMassLOD::Medium] = 6000.f;
	ReplicationTrait->Params.LODDistance[EMassLOD::Low] = 12000.f;
	ReplicationTrait->Params.LODDistance[EMassLOD::Off] = 20000.f;

	if (!Config->FindTrait(UMassWarReplicationSmoothingTrait::StaticClass()))
	{
		Config->AddTrait(UMassWarReplicationSmoothingTrait::StaticClass());
	}

	UPackage* Package = Config->GetOutermost();
	Package->MarkPackageDirty();

	const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	const bool bSaved = UPackage::SavePackage(Package, Config, *PackageFileName, SaveArgs);

	UE_LOG(LogTemp, Warning, TEXT("MassWarSetupReplication: added=%d saved=%d path=%s"), ReplicationTrait != nullptr, bSaved, *PackageFileName);

	return bSaved ? 0 : 1;
}
