// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rules/MassWarDamageRule.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassWarDamageRule)

void UMassWarCombatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UMassWarCombatSettings* Settings = GetDefault<UMassWarCombatSettings>();
	if (Settings && Settings->DamageRule)
	{
		DamageRule = NewObject<UMassWarDamageRule>(this, Settings->DamageRule);
	}
}