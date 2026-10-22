// Copyright Epic Games, Inc. All Rights Reserved.

#include "MassWarReplication.h"
#include "HAL/IConsoleManager.h"

#define LOCTEXT_NAMESPACE "FMassWarReplicationModule"

void FMassWarReplicationModule::StartupModule()
{
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 8
	// UE 5.8 runs Mass processors through a new processing queue that finds a processor's queries by address. The engine's own
	// replication processor runs a COPY of its query (one per replication shared fragment), which the queue then rejects:
	//   "Query not found! Processor attempting to run a query it doesn't own."  (a crash on the first replicated unit).
	// Until the engine handles that, fall back to the previous way of executing processors. Remove this once replication works with the queue.
	if (IConsoleVariable* UseProcessingQueue = IConsoleManager::Get().FindConsoleVariable(TEXT("mass.UseProcessingQueue")))
	{
		UseProcessingQueue->Set(0, ECVF_SetByProjectSetting);
		UE_LOG(LogTemp, Log, TEXT("MassWarReplication: mass.UseProcessingQueue set to 0 (the engine's Mass replication does not support the processing queue in this engine version)."));
	}
#endif
}

void FMassWarReplicationModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMassWarReplicationModule, MassWarReplication)
