// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/MassWarUnitCharacter.h"
#include "Controllers/MassWarUnitAIController.h"
#include "UnitBrain/MassWarUnitStateComponent.h"
#include "Fragments/MassWarUnitFragments.h"
#include "Registry/MassWarUnitRegistrySubsystem.h"
#include "MassSpawnerSubsystem.h"
#include "MassEntityManager.h"
#include "MassCommonFragments.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AMassWarUnitCharacter::AMassWarUnitCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = AMassWarUnitAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetMesh()->SetVisibility(false);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(RootComponent);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, -88.f));
	BodyMesh->SetRelativeScale3D(FVector(0.68f, 0.68f, 1.76f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderFinder.Succeeded())
	{
		BodyMesh->SetStaticMesh(CylinderFinder.Object);
	}
}

void AMassWarUnitCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TickMovement(DeltaSeconds);
	TickAttack(DeltaSeconds);
}

void AMassWarUnitCharacter::TickMovement(float DeltaSeconds)
{
	const AMassWarUnitAIController* AIController = Cast<AMassWarUnitAIController>(GetController());
	UMassWarUnitStateComponent* StateComponent = AIController ? AIController->GetStateComponent() : nullptr;
	if (!StateComponent)
	{
		return;
	}

	FMassWarOrderFragment& Order = StateComponent->Order;

	FVector Destination;
	if (Order.OrderType == EMassWarOrderType::Move)
	{
		Destination = Order.Destination;
	}
	else if (Order.OrderType == EMassWarOrderType::Attack && Order.TargetEntity.IsValid())
	{
		UMassSpawnerSubsystem* Spawner = GetWorld() ? GetWorld()->GetSubsystem<UMassSpawnerSubsystem>() : nullptr;
		if (!Spawner)
		{
			return;
		}
		FMassEntityManager& EntityManager = Spawner->GetEntityManagerChecked();
		if (!EntityManager.IsEntityValid(Order.TargetEntity))
		{
			return;
		}
		const FTransformFragment* TargetTransform = EntityManager.GetFragmentDataPtr<FTransformFragment>(Order.TargetEntity);
		if (!TargetTransform)
		{
			return;
		}
		Destination = TargetTransform->GetTransform().GetLocation();
	}
	else
	{
		return;
	}

	FVector ToDestination = Destination - GetActorLocation();
	ToDestination.Z = 0.f;

	const float Distance = ToDestination.Size();
	if (Distance <= AcceptanceRadius)
	{
		if (Order.OrderType == EMassWarOrderType::Move)
		{
			Order.OrderType = EMassWarOrderType::Idle;
			Order.bPlayerCommanded = false;
		}
		// Attack orders stay Attack once in range - TickAttack takes it from here.
		return;
	}

	AddMovementInput(ToDestination.GetSafeNormal(), 1.f);
}

void AMassWarUnitCharacter::TickAttack(float DeltaSeconds)
{
	TimeSinceLastAttack += DeltaSeconds;

	const AMassWarUnitAIController* AIController = Cast<AMassWarUnitAIController>(GetController());
	UMassWarUnitStateComponent* StateComponent = AIController ? AIController->GetStateComponent() : nullptr;
	if (!StateComponent)
	{
		return;
	}

	FMassWarOrderFragment& Order = StateComponent->Order;
	if (Order.OrderType != EMassWarOrderType::Attack || !Order.TargetEntity.IsValid())
	{
		return;
	}

	UWorld* World = GetWorld();
	UMassSpawnerSubsystem* Spawner = World ? World->GetSubsystem<UMassSpawnerSubsystem>() : nullptr;
	if (!Spawner)
	{
		return;
	}
	FMassEntityManager& EntityManager = Spawner->GetEntityManagerChecked();

	if (!EntityManager.IsEntityValid(Order.TargetEntity))
	{
		Order.OrderType = EMassWarOrderType::Idle;
		Order.TargetEntity.Reset();
		Order.bPlayerCommanded = false;
		return;
	}

	const FTransformFragment* TargetTransform = EntityManager.GetFragmentDataPtr<FTransformFragment>(Order.TargetEntity);
	if (!TargetTransform)
	{
		Order.OrderType = EMassWarOrderType::Idle;
		Order.TargetEntity.Reset();
		Order.bPlayerCommanded = false;
		return;
	}

	const float Distance = FVector::Dist(GetActorLocation(), TargetTransform->GetTransform().GetLocation());
	if (Distance > AttackRange || TimeSinceLastAttack < AttackInterval)
	{
		return;
	}

	UMassWarUnitRegistrySubsystem* Registry = World->GetSubsystem<UMassWarUnitRegistrySubsystem>();
	if (!Registry || !Registry->OnDealDamage.IsBound())
	{
		return;
	}

	TimeSinceLastAttack = 0.f;
	Registry->OnDealDamage.Execute(Order.TargetEntity, AttackDamage, FMassEntityHandle());
}
