// Copyright Epic Games, Inc. All Rights Reserved.

#include "MassWarDemoTemplateRegistrationSubsystem.h"
#include "MassEntityConfigAsset.h"
#include "UObject/SoftObjectPath.h"

void UMassWarDemoTemplateRegistrationSubsystem::PostInitialize()
{
	Super::PostInitialize();

	const FSoftObjectPath DemoUnitConfigPath(TEXT("/Game/MassWar/DA_MassWarDemoUnit.DA_MassWarDemoUnit"));
	if (UMassEntityConfigAsset* Config = Cast<UMassEntityConfigAsset>(DemoUnitConfigPath.TryLoad()))
	{
		Config->GetOrCreateEntityTemplate(*GetWorld());
	}
}
