// Copyright Epic Games, Inc. All Rights Reserved.

#include "MassWarSetupAttackAnimCommandlet.h"
#include "Characters/MassWarUnitVisualCharacter.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Engine/Blueprint.h"
#include "Factories/AnimMontageFactory.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"

namespace
{
	bool SaveAsset(UObject* Asset)
	{
		UPackage* Package = Asset->GetOutermost();
		Package->MarkPackageDirty();

		const FString FileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		return UPackage::SavePackage(Package, Asset, *FileName, SaveArgs);
	}
}

int32 UMassWarSetupAttackAnimCommandlet::Main(const FString& Params)
{
	const TCHAR* SourceSequences[] =
	{
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01"),
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02.MM_Attack_02"),
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03.MM_Attack_03"),
	};

	TArray<TObjectPtr<UAnimMontage>> Montages;
	int32 Index = 0;
	for (const TCHAR* SequencePath : SourceSequences)
	{
		++Index;
		const FString AssetName = FString::Printf(TEXT("AM_MassWar_Attack_%02d"), Index);
		const FString PackagePath = FString::Printf(TEXT("/Game/MassWar/Animation/%s"), *AssetName);
		const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName);

		if (UAnimMontage* Existing = LoadObject<UAnimMontage>(nullptr, *ObjectPath))
		{
			UE_LOG(LogTemp, Warning, TEXT("MassWarSetupAttackAnim: reusing existing montage %s"), *ObjectPath);
			Montages.Add(Existing);
			continue;
		}

		UAnimSequence* Sequence = LoadObject<UAnimSequence>(nullptr, SequencePath);
		if (!Sequence)
		{
			UE_LOG(LogTemp, Error, TEXT("MassWarSetupAttackAnim: could not load animation sequence %s"), SequencePath);
			return 1;
		}

		UPackage* Package = CreatePackage(*PackagePath);
		Package->FullyLoad();

		UAnimMontageFactory* Factory = NewObject<UAnimMontageFactory>();
		Factory->SourceAnimation = Sequence;
		Factory->TargetSkeleton = Sequence->GetSkeleton();

		UAnimMontage* Montage = Cast<UAnimMontage>(Factory->FactoryCreateNew(UAnimMontage::StaticClass(), Package, FName(*AssetName), RF_Public | RF_Standalone | RF_Transactional, nullptr, GWarn));
		if (!Montage)
		{
			UE_LOG(LogTemp, Error, TEXT("MassWarSetupAttackAnim: montage factory failed for %s"), SequencePath);
			return 1;
		}

		// Short blends so a swing reads immediately over the locomotion pose, and returns to it smoothly.
		Montage->BlendIn.SetBlendTime(0.1f);
		Montage->BlendOut.SetBlendTime(0.2f);

		FAssetRegistryModule::AssetCreated(Montage);
		if (!SaveAsset(Montage))
		{
			UE_LOG(LogTemp, Error, TEXT("MassWarSetupAttackAnim: could not save %s"), *PackagePath);
			return 1;
		}

		UE_LOG(LogTemp, Warning, TEXT("MassWarSetupAttackAnim: created %s from %s"), *ObjectPath, SequencePath);
		Montages.Add(Montage);
	}

	// Assign to the puppet Blueprint's defaults.
	const FString BlueprintPath = TEXT("/Game/MassWar/BP_MassWarUnitVisual.BP_MassWarUnitVisual");
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
	AMassWarUnitVisualCharacter* PuppetDefaults = (Blueprint && Blueprint->GeneratedClass) ? Cast<AMassWarUnitVisualCharacter>(Blueprint->GeneratedClass->GetDefaultObject()) : nullptr;
	if (!PuppetDefaults)
	{
		UE_LOG(LogTemp, Error, TEXT("MassWarSetupAttackAnim: could not load %s (or it is not an AMassWarUnitVisualCharacter)"), *BlueprintPath);
		return 1;
	}

	PuppetDefaults->AttackMontages = Montages;
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);

	if (!SaveAsset(Blueprint))
	{
		UE_LOG(LogTemp, Error, TEXT("MassWarSetupAttackAnim: could not save %s"), *BlueprintPath);
		return 1;
	}

	UE_LOG(LogTemp, Warning, TEXT("MassWarSetupAttackAnim: assigned %d attack montages to %s"), Montages.Num(), *BlueprintPath);
	return 0;
}
