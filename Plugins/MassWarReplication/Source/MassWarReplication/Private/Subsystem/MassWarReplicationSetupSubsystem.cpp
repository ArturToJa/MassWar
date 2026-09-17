// Copyright Epic Games, Inc. All Rights Reserved.

#include "Subsystem/MassWarReplicationSetupSubsystem.h"
#include "MassReplicationSubsystem.h"
#include "Replication/MassWarClientBubble.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MassWarReplicationSetupSubsystem)

void UMassWarReplicationSetupSubsystem::PostInitialize()
{
	Super::PostInitialize();

	UMassReplicationSubsystem* ReplicationSubsystem = UWorld::GetSubsystem<UMassReplicationSubsystem>(GetWorld());
	check(ReplicationSubsystem);

	ReplicationSubsystem->RegisterBubbleInfoClass(AMassWarClientBubbleInfo::StaticClass());
}
