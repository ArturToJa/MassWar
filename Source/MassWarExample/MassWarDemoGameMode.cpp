// Copyright Epic Games, Inc. All Rights Reserved.

#include "MassWarDemoGameMode.h"
#include "MassSpawnerSubsystem.h"
#include "MassEntityConfigAsset.h"
#include "MassEntityManager.h"
#include "MassCommonFragments.h"
#include "MassRepresentationFragments.h"
#include "MassActorSubsystem.h"
#include "Fragments/MassWarUnitFragments.h"
#include "Fragments/MassWarCombatFragments.h"
#include "Registry/MassWarUnitRegistrySubsystem.h"
#include "UObject/SoftObjectPath.h"
#include "TimerManager.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"
#include "Player/MassWarRTSCameraPawn.h"
#include "Player/MassWarSelectionPlayerController.h"
#include "UI/MassWarHUD.h"
#include "Characters/MassWarUnitCharacter.h"
#include "Controllers/MassWarUnitAIController.h"
#include "UnitBrain/MassWarUnitStateComponent.h"

AMassWarDemoGameMode::AMassWarDemoGameMode()
{
	DemoUnitConfigPath = FSoftObjectPath(TEXT("/Game/MassWar/DA_MassWarDemoUnit.DA_MassWarDemoUnit"));

	PlayerControllerClass = AMassWarSelectionPlayerController::StaticClass();
	DefaultPawnClass = AMassWarRTSCameraPawn::StaticClass();
	HUDClass = AMassWarHUD::StaticClass();
	HeroCharacterClass = AMassWarUnitCharacter::StaticClass();
}

void AMassWarDemoGameMode::BeginPlay()
{
	Super::BeginPlay();

	// No upfront spawn - each connecting player gets their own 150 units in HandleStartingNewPlayer_Implementation.

	GetWorldTimerManager().SetTimer(DebugDiagnosticsTimerHandle, this, &AMassWarDemoGameMode::LogEntityDiagnostics, 5.0f, true);

	if (!DebugAttackConsoleCommand)
	{
		DebugAttackConsoleCommand = IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("MassWar.DebugAttack"),
			TEXT("Test harness: orders every unit on every team to attack its nearest enemy-team unit."),
			FConsoleCommandDelegate::CreateUObject(this, &AMassWarDemoGameMode::DebugAttackNearestEnemy),
			ECVF_Default);
	}

	if (!DebugMoveConsoleCommand)
	{
		DebugMoveConsoleCommand = IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("MassWar.DebugMove"),
			TEXT("Test harness: orders the lowest-numbered team's units to move toward another team's units."),
			FConsoleCommandDelegate::CreateUObject(this, &AMassWarDemoGameMode::DebugMoveFirstTeamTowardOthers),
			ECVF_Default);
	}

	GetWorldTimerManager().SetTimer(DebugHUDTimerHandle, this, &AMassWarDemoGameMode::UpdateDebugHUD, 0.5f, true);
}

void AMassWarDemoGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (DebugAttackConsoleCommand)
	{
		IConsoleManager::Get().UnregisterConsoleObject(DebugAttackConsoleCommand);
		DebugAttackConsoleCommand = nullptr;
	}

	if (DebugMoveConsoleCommand)
	{
		IConsoleManager::Get().UnregisterConsoleObject(DebugMoveConsoleCommand);
		DebugMoveConsoleCommand = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

uint8 AMassWarDemoGameMode::AssignTeamForPlayer(int32 PlayerIndex)
{
	// Every connecting player gets their own team for now. Change only this function to re-group players
	// later - e.g. `return static_cast<uint8>(((PlayerIndex - 1) / 2) + 1);` would pair players up
	// two-at-a-time onto shared teams instead.
	return static_cast<uint8>(PlayerIndex);
}

FVector AMassWarDemoGameMode::ComputeSpawnOriginForPlayer(int32 PlayerIndex) const
{
	// Alternates left/right of the map center (1st player left, 2nd right, matching every earlier pass's
	// 2-player test setup exactly), stepping a further TeamOriginOffset out for every second player so
	// slots never overlap: 1 -> -4000, 2 -> +4000, 3 -> -8000, 4 -> +8000, ...
	const int32 Slot = (PlayerIndex + 1) / 2;
	const float Side = (PlayerIndex % 2 == 1) ? -1.f : 1.f;
	return FVector(Side * TeamOriginOffset * Slot, 0.f, 100.f);
}

UMassEntityConfigAsset* AMassWarDemoGameMode::GetOrLoadDemoUnitConfig()
{
	if (!CachedDemoUnitConfig)
	{
		CachedDemoUnitConfig = Cast<UMassEntityConfigAsset>(DemoUnitConfigPath.TryLoad());
		if (!CachedDemoUnitConfig)
		{
			UE_LOG(LogTemp, Error, TEXT("MassWarDemoGameMode: could not load demo unit config at %s"), *DemoUnitConfigPath.ToString());
		}
	}
	return CachedDemoUnitConfig;
}

void AMassWarDemoGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	if (AMassWarSelectionPlayerController* MassWarPC = Cast<AMassWarSelectionPlayerController>(NewPlayer))
	{
		const int32 PlayerIndex = NextPlayerIndex++;
		const uint32 PlayerId = NextPlayerId++;
		const uint8 TeamId = AssignTeamForPlayer(PlayerIndex);

		MassWarPC->SetPlayerId(PlayerId);
		MassWarPC->SetPlayerTeamId(TeamId);

		UE_LOG(LogTemp, Log, TEXT("MassWarDemoGameMode: player #%d (id=%u) assigned to team %d"), PlayerIndex, PlayerId, TeamId);

		if (UMassEntityConfigAsset* Config = GetOrLoadDemoUnitConfig())
		{
			const FVector Origin = ComputeSpawnOriginForPlayer(PlayerIndex);
			SpawnUnitsForPlayer(*Config, PlayerId, TeamId, Origin, UnitsPerPlayer);

			if (PlayerIndex == 1)
			{
				SpawnHeroForPlayer(TeamId, PlayerId, Origin);
			}
		}
	}

	if (APawn* Pawn = NewPlayer ? NewPlayer->GetPawn() : nullptr)
	{
		// AMassWarRTSCameraPawn's own CameraComponent carries a fixed downward relative pitch, so the
		// pawn/actor rotation itself should stay identity here - setting an actor-level pitch too would
		// double up (actor pitch + camera's own relative pitch) and point the view the wrong way.
		Pawn->SetActorLocation(FVector(0.f, 0.f, DemoCameraHeight));
		UE_LOG(LogTemp, Log, TEXT("MassWarDemoGameMode: moved demo camera to (0, 0, %.0f)"), DemoCameraHeight);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MassWarDemoGameMode: no pawn to reposition for the demo camera - navigate to the spawn area manually"));
	}
}

void AMassWarDemoGameMode::SpawnUnitsForPlayer(UMassEntityConfigAsset& Config, uint32 PlayerId, uint8 TeamId, const FVector& Origin, int32 Count)
{
	UWorld* World = GetWorld();
	UMassSpawnerSubsystem* Spawner = World ? World->GetSubsystem<UMassSpawnerSubsystem>() : nullptr;
	if (!Spawner)
	{
		UE_LOG(LogTemp, Error, TEXT("MassWarDemoGameMode: no UMassSpawnerSubsystem available"));
		return;
	}

	const FMassEntityTemplate& Template = Config.GetOrCreateEntityTemplate(*World);

	TArray<FMassEntityHandle> SpawnedEntities;
	Spawner->SpawnEntities(Template, static_cast<uint32>(Count), SpawnedEntities);

	FMassEntityManager& EntityManager = Spawner->GetEntityManagerChecked();
	UMassWarUnitRegistrySubsystem* Registry = World->GetSubsystem<UMassWarUnitRegistrySubsystem>();
	TArray<FMassEntityHandle>& TeamEntities = EntitiesByTeam.FindOrAdd(TeamId);

	for (const FMassEntityHandle& Entity : SpawnedEntities)
	{
		if (Registry)
		{
			Registry->RegisterUnit(Entity);
		}

		if (FMassWarTeamFragment* Team = EntityManager.GetFragmentDataPtr<FMassWarTeamFragment>(Entity))
		{
			Team->TeamId = TeamId;
		}

		if (FMassWarOwnerFragment* OwnerFragment = EntityManager.GetFragmentDataPtr<FMassWarOwnerFragment>(Entity))
		{
			OwnerFragment->OwningPlayerId = PlayerId;
		}

		if (FTransformFragment* Transform = EntityManager.GetFragmentDataPtr<FTransformFragment>(Entity))
		{
			const FVector RandomOffset(
				FMath::FRandRange(-SpawnAreaExtent, SpawnAreaExtent),
				FMath::FRandRange(-SpawnAreaExtent, SpawnAreaExtent),
				0.f);
			Transform->SetTransform(FTransform(Origin + RandomOffset));
		}

		if (DebugSampleEntities.Num() < 5)
		{
			DebugSampleEntities.Add(Entity);
		}

		TeamEntities.Add(Entity);
	}

	UE_LOG(LogTemp, Log, TEXT("MassWarDemoGameMode: spawned %d units for player %u (team %d)"), SpawnedEntities.Num(), PlayerId, TeamId);
}

void AMassWarDemoGameMode::SpawnHeroForPlayer(uint8 TeamId, uint32 PlayerId, const FVector& Origin)
{
	if (bHeroSpawned || !HeroCharacterClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const FVector SpawnLocation = Origin + FVector(0.f, 0.f, 100.f);
	APawn* HeroPawn = World->SpawnActor<APawn>(HeroCharacterClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);
	if (!HeroPawn)
	{
		UE_LOG(LogTemp, Error, TEXT("MassWarDemoGameMode: failed to spawn hero for player %u"), PlayerId);
		return;
	}

	AMassWarUnitAIController* HeroController = Cast<AMassWarUnitAIController>(HeroPawn->GetController());
	if (UMassWarUnitStateComponent* StateComponent = HeroController ? HeroController->GetStateComponent() : nullptr)
	{
		StateComponent->TeamId = TeamId;
		StateComponent->OwningPlayerId = PlayerId;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("MassWarDemoGameMode: hero for player %u has no AMassWarUnitAIController/UMassWarUnitStateComponent yet (not possessed at spawn time?) - team/owner not set"), PlayerId);
	}

	bHeroSpawned = true;
	UE_LOG(LogTemp, Log, TEXT("MassWarDemoGameMode: spawned hero for player %u (team %d) at %s"), PlayerId, TeamId, *SpawnLocation.ToString());
}

void AMassWarDemoGameMode::DebugAttackNearestEnemy()
{
	UMassSpawnerSubsystem* Spawner = GetWorld() ? GetWorld()->GetSubsystem<UMassSpawnerSubsystem>() : nullptr;
	if (!Spawner)
	{
		return;
	}

	FMassEntityManager& EntityManager = Spawner->GetEntityManagerChecked();
	int32 OrdersIssued = 0;

	for (const TPair<uint8, TArray<FMassEntityHandle>>& AttackerTeamPair : EntitiesByTeam)
	{
		for (const FMassEntityHandle& Attacker : AttackerTeamPair.Value)
		{
			if (!EntityManager.IsEntityValid(Attacker))
			{
				continue;
			}

			const FTransformFragment* AttackerTransform = EntityManager.GetFragmentDataPtr<FTransformFragment>(Attacker);
			FMassWarOrderFragment* AttackerOrder = EntityManager.GetFragmentDataPtr<FMassWarOrderFragment>(Attacker);
			if (!AttackerTransform || !AttackerOrder)
			{
				continue;
			}

			FMassEntityHandle NearestEnemy;
			float NearestDistSq = TNumericLimits<float>::Max();
			const FVector AttackerLocation = AttackerTransform->GetTransform().GetLocation();

			for (const TPair<uint8, TArray<FMassEntityHandle>>& CandidateTeamPair : EntitiesByTeam)
			{
				if (CandidateTeamPair.Key == AttackerTeamPair.Key)
				{
					continue;
				}

				for (const FMassEntityHandle& Candidate : CandidateTeamPair.Value)
				{
					if (!EntityManager.IsEntityValid(Candidate))
					{
						continue;
					}

					const FTransformFragment* CandidateTransform = EntityManager.GetFragmentDataPtr<FTransformFragment>(Candidate);
					if (!CandidateTransform)
					{
						continue;
					}

					const float DistSq = FVector::DistSquared(AttackerLocation, CandidateTransform->GetTransform().GetLocation());
					if (DistSq < NearestDistSq)
					{
						NearestDistSq = DistSq;
						NearestEnemy = Candidate;
					}
				}
			}

			if (NearestEnemy.IsValid())
			{
				AttackerOrder->OrderType = EMassWarOrderType::Attack;
				AttackerOrder->TargetEntity = NearestEnemy;
				++OrdersIssued;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("MassWarDemoGameMode: DebugAttackNearestEnemy issued %d attack orders"), OrdersIssued);
}

void AMassWarDemoGameMode::DebugMoveFirstTeamTowardOthers()
{
	UMassSpawnerSubsystem* Spawner = GetWorld() ? GetWorld()->GetSubsystem<UMassSpawnerSubsystem>() : nullptr;
	if (!Spawner || EntitiesByTeam.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("MassWarDemoGameMode: DebugMoveFirstTeamTowardOthers needs at least 2 teams spawned"));
		return;
	}

	TArray<uint8> TeamIds;
	EntitiesByTeam.GetKeys(TeamIds);
	TeamIds.Sort();

	const uint8 SourceTeamId = TeamIds[0];
	const uint8 TargetTeamId = TeamIds[1];

	FMassEntityManager& EntityManager = Spawner->GetEntityManagerChecked();

	FVector Destination = FVector::ZeroVector;
	int32 AliveCount = 0;
	for (const FMassEntityHandle& Entity : EntitiesByTeam[TargetTeamId])
	{
		const FTransformFragment* Transform = EntityManager.IsEntityValid(Entity) ? EntityManager.GetFragmentDataPtr<FTransformFragment>(Entity) : nullptr;
		if (Transform)
		{
			Destination += Transform->GetTransform().GetLocation();
			++AliveCount;
		}
	}

	if (AliveCount == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("MassWarDemoGameMode: DebugMoveFirstTeamTowardOthers - target team %d has no living units"), TargetTeamId);
		return;
	}
	Destination /= AliveCount;

	int32 OrdersIssued = 0;
	for (const FMassEntityHandle& Entity : EntitiesByTeam[SourceTeamId])
	{
		if (!EntityManager.IsEntityValid(Entity))
		{
			continue;
		}

		if (FMassWarOrderFragment* Order = EntityManager.GetFragmentDataPtr<FMassWarOrderFragment>(Entity))
		{
			Order->OrderType = EMassWarOrderType::Move;
			Order->Destination = Destination;
			Order->TargetEntity.Reset();
			++OrdersIssued;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("MassWarDemoGameMode: DebugMoveFirstTeamTowardOthers issued %d move orders (team %d -> team %d) toward %s"),
		OrdersIssued, SourceTeamId, TargetTeamId, *Destination.ToString());
}

void AMassWarDemoGameMode::UpdateDebugHUD()
{
	UMassSpawnerSubsystem* Spawner = GetWorld() ? GetWorld()->GetSubsystem<UMassSpawnerSubsystem>() : nullptr;
	if (!Spawner || !GEngine)
	{
		return;
	}

	FMassEntityManager& EntityManager = Spawner->GetEntityManagerChecked();

	TArray<uint8> TeamIds;
	EntitiesByTeam.GetKeys(TeamIds);
	TeamIds.Sort();

	FString Message = TEXT("(console: MassWar.DebugAttack)  ");
	for (const uint8 TeamId : TeamIds)
	{
		const TArray<FMassEntityHandle>& Entities = EntitiesByTeam[TeamId];
		int32 Alive = 0;
		for (const FMassEntityHandle& Entity : Entities)
		{
			if (EntityManager.IsEntityValid(Entity))
			{
				++Alive;
			}
		}
		Message += FString::Printf(TEXT("Team %d: %d / %d alive    "), TeamId, Alive, Entities.Num());
	}

	GEngine->AddOnScreenDebugMessage(/*Key=*/ 100, /*TimeToDisplay=*/ 1.f, FColor::Yellow, Message);
}

void AMassWarDemoGameMode::LogEntityDiagnostics()
{
	UMassSpawnerSubsystem* Spawner = GetWorld() ? GetWorld()->GetSubsystem<UMassSpawnerSubsystem>() : nullptr;
	if (!Spawner)
	{
		return;
	}

	FMassEntityManager& EntityManager = Spawner->GetEntityManagerChecked();
	UE_LOG(LogTemp, Warning, TEXT("MassWarDemoGameMode DIAGNOSTICS: dumping %d sample entities"), DebugSampleEntities.Num());

	for (const FMassEntityHandle& Entity : DebugSampleEntities)
	{
		if (!EntityManager.IsEntityValid(Entity))
		{
			UE_LOG(LogTemp, Warning, TEXT("  Entity %s: NOT VALID (destroyed?)"), *Entity.DebugGetDescription());
			continue;
		}

		const FTransformFragment* Transform = EntityManager.GetFragmentDataPtr<FTransformFragment>(Entity);
		const FMassRepresentationFragment* Rep = EntityManager.GetFragmentDataPtr<FMassRepresentationFragment>(Entity);
		const FMassRepresentationLODFragment* RepLOD = EntityManager.GetFragmentDataPtr<FMassRepresentationLODFragment>(Entity);
		const FMassActorFragment* ActorFrag = EntityManager.GetFragmentDataPtr<FMassActorFragment>(Entity);
		const FMassWarCombatParamsFragment* Combat = EntityManager.GetFragmentDataPtr<FMassWarCombatParamsFragment>(Entity);
		const FMassWarHealthFragment* Health = EntityManager.GetFragmentDataPtr<FMassWarHealthFragment>(Entity);

		UE_LOG(LogTemp, Warning, TEXT("  Entity %s: Loc=%s HasRepFragment=%d HasLODFragment=%d HasActorFragment=%d CurrentRep=%s LOD=%d Visibility=%d Significance=%.2f StaticMeshDescHandle=%d ActorValid=%d HasCombatFragment=%d AttackRange=%.1f AttackDamage=%.1f AttackInterval=%.2f Health=%.1f/%.1f"),
			*Entity.DebugGetDescription(),
			Transform ? *Transform->GetTransform().GetLocation().ToString() : TEXT("NONE"),
			Rep != nullptr,
			RepLOD != nullptr,
			ActorFrag != nullptr,
			Rep ? *UEnum::GetValueAsString(Rep->CurrentRepresentation) : TEXT("N/A"),
			RepLOD ? static_cast<int32>(RepLOD->LOD.GetValue()) : -1,
			RepLOD ? static_cast<int32>(RepLOD->Visibility) : -1,
			RepLOD ? RepLOD->LODSignificance : -1.f,
			Rep ? Rep->StaticMeshDescHandle.ToIndex() : -1,
			ActorFrag && ActorFrag->Get() != nullptr,
			Combat != nullptr,
			Combat ? Combat->AttackRange : -1.f,
			Combat ? Combat->AttackDamage : -1.f,
			Combat ? Combat->AttackInterval : -1.f,
			Health ? Health->Health : -1.f,
			Health ? Health->MaxHealth : -1.f);
	}
}
