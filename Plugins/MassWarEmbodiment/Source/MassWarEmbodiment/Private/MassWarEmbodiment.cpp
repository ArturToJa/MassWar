// Copyright Epic Games, Inc. All Rights Reserved.

#include "MassWarEmbodiment.h"
#include "MassUpdateISMProcessor.h"
#include "MassProcessor.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "FMassWarEmbodimentModule"

void FMassWarEmbodimentModule::StartupModule()
{
	// UMassWarUpdateISMProcessor does everything the engine's UMassUpdateISMProcessor does (plus the far-unit
	// animation custom data), so the engine's one must not run as well - both would push every transform.
	// Set in memory through reflection, NOT through UMassProcessor::SetShouldAutoRegisterWithGlobalList: that one also
	// writes the value into the project's DefaultMass.ini in editor builds, and Mass config sections are inherited,
	// so the saved entry would silently switch off any processor derived from the engine's class as well.
	if (UMassUpdateISMProcessor* EngineProcessor = GetMutableDefault<UMassUpdateISMProcessor>())
	{
		if (const FBoolProperty* AutoRegister = CastField<FBoolProperty>(UMassProcessor::StaticClass()->FindPropertyByName(TEXT("bAutoRegisterWithProcessingPhases"))))
		{
			AutoRegister->SetPropertyValue_InContainer(EngineProcessor, false);
		}
	}
}

void FMassWarEmbodimentModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMassWarEmbodimentModule, MassWarEmbodiment)
