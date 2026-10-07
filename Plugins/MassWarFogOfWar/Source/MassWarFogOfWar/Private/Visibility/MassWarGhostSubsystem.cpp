// Copyright Epic Games, Inc. All Rights Reserved.

#include "Visibility/MassWarGhostSubsystem.h"
#include "Subsystem/MassWarReplicationSetupSubsystem.h"
#include "Interfaces/MassWarTeamProviderInterface.h"
#include "Fragments/MassWarUnitFragments.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<float> CVarGhostLifetime(
		TEXT("MassWar.Ghosts.Lifetime"), 10.f,
		TEXT("Seconds a fog-of-war ghost (last known position of an enemy that went out of sight) is kept."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarGhostDraw(
		TEXT("MassWar.Ghosts.Draw"), 0,
		TEXT("1 = draw a simple debug marker at every fog-of-war ghost (for testing; real games bind OnGhostAdded)."),
		ECVF_Default);
}

bool UMassWarGhostSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && !World->IsNetMode(NM_DedicatedServer);
}

void UMassWarGhostSubsystem::PostInitialize()
{
	Super::PostInitialize();

	if (UMassWarReplicationSetupSubsystem* Setup = GetWorld()->GetSubsystem<UMassWarReplicationSetupSubsystem>())
	{
		RemovedHandle = Setup->OnClientAgentRemoved.AddUObject(this, &UMassWarGhostSubsystem::HandleClientAgentRemoved);
		AddedHandle = Setup->OnClientAgentAdded.AddUObject(this, &UMassWarGhostSubsystem::HandleClientAgentAdded);
	}
}

void UMassWarGhostSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		if (UMassWarReplicationSetupSubsystem* Setup = World->GetSubsystem<UMassWarReplicationSetupSubsystem>())
		{
			Setup->OnClientAgentRemoved.Remove(RemovedHandle);
			Setup->OnClientAgentAdded.Remove(AddedHandle);
		}
	}
	Super::Deinitialize();
}

uint8 UMassWarGhostSubsystem::GetLocalTeamId() const
{
	if (const UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* PC = It->Get();
			if (PC && PC->IsLocalController())
			{
				if (const IMassWarTeamProvider* TeamProvider = Cast<IMassWarTeamProvider>(PC))
				{
					return TeamProvider->GetMassWarPlayerTeamId();
				}
			}
		}
	}
	return 0;
}

void UMassWarGhostSubsystem::AddGhost(const FMassWarGhost& Ghost)
{
	Ghosts.Add(Ghost);
	OnGhostAdded.Broadcast(Ghost);
}

void UMassWarGhostSubsystem::RemoveGhostAt(int32 Index)
{
	const FMassWarGhost Removed = Ghosts[Index];
	Ghosts.RemoveAtSwap(Index);
	OnGhostRemoved.Broadcast(Removed);
}

void UMassWarGhostSubsystem::HandleClientAgentRemoved(uint32 NetId, const FVector& Location, float YawDegrees, uint8 TeamId, uint8 LifeState)
{
	// Only enemies that left sight alive: our own units, neutrals and dying units (which are removed once their
	// death animation is done) leave no ghost.
	const uint8 LocalTeamId = GetLocalTeamId();
	if (TeamId == 0 || TeamId == LocalTeamId || LifeState != static_cast<uint8>(EMassWarLifeState::Alive))
	{
		return;
	}

	FMassWarGhost Ghost;
	Ghost.Location = Location;
	Ghost.YawDegrees = YawDegrees;
	Ghost.TeamId = TeamId;
	Ghost.NetId = NetId;
	AddGhost(Ghost);
}

void UMassWarGhostSubsystem::HandleClientAgentAdded(uint32 NetId)
{
	for (int32 Index = Ghosts.Num() - 1; Index >= 0; --Index)
	{
		if (Ghosts[Index].NetId == NetId)
		{
			RemoveGhostAt(Index);
		}
	}
}

void UMassWarGhostSubsystem::AddGhostForEntity(FMassEntityHandle Entity, const FVector& Location, float YawDegrees, uint8 TeamId)
{
	ClearGhostForEntity(Entity);

	FMassWarGhost Ghost;
	Ghost.Location = Location;
	Ghost.YawDegrees = YawDegrees;
	Ghost.TeamId = TeamId;
	Ghost.Entity = Entity;
	AddGhost(Ghost);
}

void UMassWarGhostSubsystem::ClearGhostForEntity(FMassEntityHandle Entity)
{
	for (int32 Index = Ghosts.Num() - 1; Index >= 0; --Index)
	{
		if (Ghosts[Index].Entity == Entity && Entity.IsValid())
		{
			RemoveGhostAt(Index);
		}
	}
}

void UMassWarGhostSubsystem::Tick(float DeltaTime)
{
	const float Lifetime = CVarGhostLifetime.GetValueOnGameThread();
	const bool bDraw = CVarGhostDraw.GetValueOnGameThread() != 0;
	UWorld* World = GetWorld();

	for (int32 Index = Ghosts.Num() - 1; Index >= 0; --Index)
	{
		FMassWarGhost& Ghost = Ghosts[Index];
		Ghost.Age += DeltaTime;
		if (Ghost.Age >= Lifetime)
		{
			RemoveGhostAt(Index);
			continue;
		}
		if (bDraw && World)
		{
			// Fades from a solid red towards grey as it ages.
			const float Fade = 1.f - Ghost.Age / FMath::Max(Lifetime, 0.01f);
			const FColor Color(static_cast<uint8>(110 + 145 * Fade), static_cast<uint8>(110 - 70 * Fade), static_cast<uint8>(110 - 70 * Fade));
			const FVector Facing = FRotator(0.f, Ghost.YawDegrees, 0.f).Vector() * 60.f;
			DrawDebugCylinder(World, Ghost.Location, Ghost.Location + FVector(0, 0, 180), 35.f, 10, Color, false, 0.f, 0, 3.f);
			DrawDebugLine(World, Ghost.Location + FVector(0, 0, 90), Ghost.Location + FVector(0, 0, 90) + Facing, Color, false, 0.f, 0, 3.f);
		}
	}
}
