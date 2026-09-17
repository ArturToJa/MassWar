// Copyright Epic Games, Inc. All Rights Reserved.

#include "MassWarSetupFogOfWarCommandlet.h"
#include "MassEntityConfigAsset.h"
#include "Traits/MassWarVisibilityTrait.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"

int32 UMassWarSetupFogOfWarCommandlet::Main(const FString& Params)
{
	const FString AssetPath = TEXT("/Game/MassWar/DA_MassWarDemoUnit.DA_MassWarDemoUnit");
	const FString PackagePath = TEXT("/Game/MassWar/DA_MassWarDemoUnit");

	UMassEntityConfigAsset* Config = LoadObject<UMassEntityConfigAsset>(nullptr, *AssetPath);
	if (!Config)
	{
		UE_LOG(LogTemp, Error, TEXT("MassWarSetupFogOfWar: could not load %s"), *AssetPath);
		return 1;
	}

	UMassWarVisibilityTrait* VisibilityTrait = nullptr;
	if (const UMassEntityTraitBase* Existing = Config->FindTrait(UMassWarVisibilityTrait::StaticClass()))
	{
		VisibilityTrait = const_cast<UMassWarVisibilityTrait*>(Cast<UMassWarVisibilityTrait>(Existing));
	}
	else
	{
		VisibilityTrait = Cast<UMassWarVisibilityTrait>(Config->AddTrait(UMassWarVisibilityTrait::StaticClass()));
	}

	if (!VisibilityTrait)
	{
		UE_LOG(LogTemp, Error, TEXT("MassWarSetupFogOfWar: failed to add/find UMassWarVisibilityTrait on %s"), *AssetPath);
		return 1;
	}

	UPackage* Package = Config->GetOutermost();
	Package->MarkPackageDirty();

	const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	const bool bSaved = UPackage::SavePackage(Package, Config, *PackageFileName, SaveArgs);

	UE_LOG(LogTemp, Warning, TEXT("MassWarSetupFogOfWar: added=%d saved=%d path=%s"), VisibilityTrait != nullptr, bSaved, *PackageFileName);

	return bSaved ? 0 : 1;
}
