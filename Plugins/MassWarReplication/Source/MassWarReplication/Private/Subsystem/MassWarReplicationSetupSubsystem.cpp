// Copyright Epic Games, Inc. All Rights Reserved.

#include "Subsystem/MassWarReplicationSetupSubsystem.h"
#include "MassReplicationSubsystem.h"
#include "Replication/MassWarClientBubble.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassWarReplicationSetupSubsystem)

void UMassWarReplicationSetupSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Makes sure UMassReplicationSubsystem is initialized (and so exists) before this subsystem.
	Collection.InitializeDependency<UMassReplicationSubsystem>();

	UMassReplicationSubsystem* ReplicationSubsystem = UWorld::GetSubsystem<UMassReplicationSubsystem>(GetWorld());
	check(ReplicationSubsystem);

	ReplicationSubsystem->RegisterBubbleInfoClass(AMassWarClientBubbleInfo::StaticClass());
}
