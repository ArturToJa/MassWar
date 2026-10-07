// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/MassWarSelectionPlayerController.h"
#include "Player/MassWarUnitOrderComponent.h"
#include "Selection/MassWarSelectionSubsystem.h"
#include "Registry/MassWarUnitRegistrySubsystem.h"
#include "Fragments/MassWarUnitFragments.h"
#include "UnitBrain/MassWarUnitStateView.h"
#include "MassSpawnerSubsystem.h"
#include "MassEntityManager.h"
#include "MassCommonFragments.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/LocalPlayer.h"
#include "Net/UnrealNetwork.h"

AMassWarSelectionPlayerController::AMassWarSelectionPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = true;
	bEnableClickEvents = true;

	OrderComponent = CreateDefaultSubobject<UMassWarUnitOrderComponent>(TEXT("OrderComponent"));
}

void AMassWarSelectionPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMassWarSelectionPlayerController, PlayerTeamId);
	DOREPLIFETIME(AMassWarSelectionPlayerController, PlayerId);
}

void AMassWarSelectionPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsLeftMouseDown)
	{
		float MouseX = 0.f, MouseY = 0.f;
		if (GetMousePosition(MouseX, MouseY))
		{
			CurrentScreenPos = FVector2D(MouseX, MouseY);
			if (!bIsDragging && FVector2D::Distance(DragStartScreenPos, CurrentScreenPos) > DragThresholdPixels)
			{
				bIsDragging = true;
			}
		}
	}
}

void AMassWarSelectionPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindAction(TEXT("LeftClick"), IE_Pressed, this, &AMassWarSelectionPlayerController::OnLeftClickPressed);
	InputComponent->BindAction(TEXT("LeftClick"), IE_Released, this, &AMassWarSelectionPlayerController::OnLeftClickReleased);
	InputComponent->BindAction(TEXT("RightClick"), IE_Pressed, this, &AMassWarSelectionPlayerController::OnRightClickPressed);
}

void AMassWarSelectionPlayerController::OnLeftClickPressed()
{
	bIsLeftMouseDown = true;
	bIsDragging = false;

	float MouseX = 0.f, MouseY = 0.f;
	if (GetMousePosition(MouseX, MouseY))
	{
		DragStartScreenPos = FVector2D(MouseX, MouseY);
		CurrentScreenPos = DragStartScreenPos;
	}
}

void AMassWarSelectionPlayerController::OnLeftClickReleased()
{
	bIsLeftMouseDown = false;

	ULocalPlayer* LP = GetLocalPlayer();
	UMassWarSelectionSubsystem* SelectionSubsystem = LP ? LP->GetSubsystem<UMassWarSelectionSubsystem>() : nullptr;
	if (!SelectionSubsystem)
	{
		bIsDragging = false;
		return;
	}

	if (bIsDragging)
	{
		const FVector2D Min(FMath::Min(DragStartScreenPos.X, CurrentScreenPos.X), FMath::Min(DragStartScreenPos.Y, CurrentScreenPos.Y));
		const FVector2D Max(FMath::Max(DragStartScreenPos.X, CurrentScreenPos.X), FMath::Max(DragStartScreenPos.Y, CurrentScreenPos.Y));
		SelectionSubsystem->SetSelection(ExpandToFormations(FindUnitsInScreenRect(Min, Max, /*bOwnedOnly=*/true)));
	}
	else
	{
		const FMassEntityHandle Clicked = FindNearestUnitAtScreenPos(CurrentScreenPos, ClickPixelTolerance);
		bool bSelected = false;
		if (Clicked.IsValid())
		{
			FMassEntityManager* EntityManager = GetEntityManager();
			const FMassWarOwnerFragment* OwnerFragment = EntityManager ? EntityManager->GetFragmentDataPtr<FMassWarOwnerFragment>(Clicked) : nullptr;
			if (OwnerFragment && OwnerFragment->OwningPlayerId == PlayerId)
			{
				TArray<FMassEntityHandle> Single;
				Single.Add(Clicked);
				SelectionSubsystem->SetSelection(ExpandToFormations(Single));
				bSelected = true;
			}
		}

		if (!bSelected)
		{
			SelectionSubsystem->ClearSelection();
		}
	}

	bIsDragging = false;
}

void AMassWarSelectionPlayerController::OnRightClickPressed()
{
	ULocalPlayer* LP = GetLocalPlayer();
	UMassWarSelectionSubsystem* SelectionSubsystem = LP ? LP->GetSubsystem<UMassWarSelectionSubsystem>() : nullptr;
	if (!SelectionSubsystem || !SelectionSubsystem->HasSelection() || !OrderComponent)
	{
		return;
	}

	float MouseX = 0.f, MouseY = 0.f;
	if (!GetMousePosition(MouseX, MouseY))
	{
		return;
	}
	const FVector2D ScreenPos(MouseX, MouseY);

	FMassEntityManager* EntityManager = GetEntityManager();
	if (!EntityManager)
	{
		return;
	}

	const FMassEntityHandle Target = FindNearestUnitAtScreenPos(ScreenPos, ClickPixelTolerance);
	if (Target.IsValid())
	{
		const FMassWarTeamFragment* Team = EntityManager->GetFragmentDataPtr<FMassWarTeamFragment>(Target);
		if (Team && Team->TeamId != PlayerTeamId && Team->TeamId != 0)
		{
			TArray<int32> FormationIds;
			TArray<FMassEntityHandle> Unformed;
			CollectFormationIds(SelectionSubsystem->GetSelection(), FormationIds, Unformed);

			// Attacking a unit that is in a formation means attacking that formation - ours then assigns the
			// targets. A lone enemy unit (no formation) is attacked directly by every selected unit, as before.
			const FMassWarFormationMemberFragment* TargetFormation = EntityManager->GetFragmentDataPtr<FMassWarFormationMemberFragment>(Target);
			if (TargetFormation && TargetFormation->FormationId != 0)
			{
				if (!FormationIds.IsEmpty())
				{
					OrderComponent->ServerIssueFormationAttackOrder(FormationIds, static_cast<int32>(TargetFormation->FormationId));
				}
				if (!Unformed.IsEmpty())
				{
					OrderComponent->ServerIssueAttackOrder(MakeOrderTargets(*EntityManager, Unformed), MakeOrderTarget(*EntityManager, Target));
				}
			}
			else
			{
				OrderComponent->ServerIssueAttackOrder(MakeOrderTargets(*EntityManager, SelectionSubsystem->GetSelection()), MakeOrderTarget(*EntityManager, Target));
			}
			return;
		}
	}

	FVector WorldLocation, WorldDirection;
	if (!DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		return;
	}

	FHitResult Hit;
	const FVector TraceEnd = WorldLocation + WorldDirection * 100000.f;
	if (GetWorld()->LineTraceSingleByChannel(Hit, WorldLocation, TraceEnd, ECC_Visibility))
	{
		TArray<int32> FormationIds;
		TArray<FMassEntityHandle> Unformed;
		CollectFormationIds(SelectionSubsystem->GetSelection(), FormationIds, Unformed);
		if (!FormationIds.IsEmpty())
		{
			OrderComponent->ServerIssueFormationMoveOrder(FormationIds, Hit.Location);
		}
		if (!Unformed.IsEmpty())
		{
			OrderComponent->ServerIssueMoveOrder(MakeOrderTargets(*EntityManager, Unformed), Hit.Location);
		}
	}
}

TArray<FMassEntityHandle> AMassWarSelectionPlayerController::ExpandToFormations(const TArray<FMassEntityHandle>& Units) const
{
	FMassEntityManager* EntityManager = GetEntityManager();
	UMassWarUnitRegistrySubsystem* Registry = GetWorld() ? GetWorld()->GetSubsystem<UMassWarUnitRegistrySubsystem>() : nullptr;
	if (!EntityManager || !Registry)
	{
		return Units;
	}

	TArray<int32> FormationIds;
	TArray<FMassEntityHandle> Unformed;
	CollectFormationIds(Units, FormationIds, Unformed);
	if (FormationIds.IsEmpty())
	{
		return Units;
	}

	TArray<FMassEntityHandle> Result = Unformed;
	for (const FMassEntityHandle& Entity : Registry->GetAllUnits())
	{
		if (!FMassWarUnitStateView::IsLiving(*EntityManager, Entity))
		{
			continue;
		}
		const FMassWarFormationMemberFragment* Member = EntityManager->GetFragmentDataPtr<FMassWarFormationMemberFragment>(Entity);
		const FMassWarOwnerFragment* OwnerFragment = EntityManager->GetFragmentDataPtr<FMassWarOwnerFragment>(Entity);
		if (Member && OwnerFragment && OwnerFragment->OwningPlayerId == PlayerId && FormationIds.Contains(static_cast<int32>(Member->FormationId)))
		{
			Result.Add(Entity);
		}
	}
	return Result;
}

void AMassWarSelectionPlayerController::CollectFormationIds(const TArray<FMassEntityHandle>& Units, TArray<int32>& OutFormationIds, TArray<FMassEntityHandle>& OutUnformedUnits) const
{
	FMassEntityManager* EntityManager = GetEntityManager();
	if (!EntityManager)
	{
		return;
	}
	for (const FMassEntityHandle& Unit : Units)
	{
		if (!FMassWarUnitStateView::IsLiving(*EntityManager, Unit))
		{
			continue;
		}
		const FMassWarFormationMemberFragment* Member = EntityManager->GetFragmentDataPtr<FMassWarFormationMemberFragment>(Unit);
		if (Member && Member->FormationId != 0)
		{
			OutFormationIds.AddUnique(static_cast<int32>(Member->FormationId));
		}
		else
		{
			OutUnformedUnits.Add(Unit);
		}
	}
}

FMassWarOrderTarget AMassWarSelectionPlayerController::MakeOrderTarget(FMassEntityManager& EntityManager, FMassEntityHandle Entity) const
{
	FMassWarOrderTarget OrderTarget;

	// A selected entity can go stale between selection and order (e.g. it died - selection is never
	// pruned when a unit is destroyed) - EntityManager doesn't allow querying a dead entity's fragments.
	if (!EntityManager.IsEntityValid(Entity))
	{
		return OrderTarget;
	}

	OrderTarget.Handle = Entity;

	if (const FMassWarNetIdFragment* NetIdFragment = EntityManager.GetFragmentDataPtr<FMassWarNetIdFragment>(Entity))
	{
		OrderTarget.NetId = NetIdFragment->NetId;
	}

	return OrderTarget;
}

TArray<FMassWarOrderTarget> AMassWarSelectionPlayerController::MakeOrderTargets(FMassEntityManager& EntityManager, const TArray<FMassEntityHandle>& Entities) const
{
	TArray<FMassWarOrderTarget> OrderTargets;
	OrderTargets.Reserve(Entities.Num());

	for (const FMassEntityHandle& Entity : Entities)
	{
		if (EntityManager.IsEntityValid(Entity))
		{
			OrderTargets.Add(MakeOrderTarget(EntityManager, Entity));
		}
	}

	return OrderTargets;
}

TArray<FMassEntityHandle> AMassWarSelectionPlayerController::FindUnitsInScreenRect(const FVector2D& Min, const FVector2D& Max, bool bOwnedOnly) const
{
	TArray<FMassEntityHandle> Result;

	FMassEntityManager* EntityManager = GetEntityManager();
	UMassWarUnitRegistrySubsystem* Registry = GetWorld() ? GetWorld()->GetSubsystem<UMassWarUnitRegistrySubsystem>() : nullptr;
	if (!EntityManager || !Registry)
	{
		return Result;
	}

	for (const FMassEntityHandle& Entity : Registry->GetAllUnits())
	{
		if (!FMassWarUnitStateView::IsLiving(*EntityManager, Entity))
		{
			continue;
		}

		if (bOwnedOnly)
		{
			const FMassWarOwnerFragment* OwnerFragment = EntityManager->GetFragmentDataPtr<FMassWarOwnerFragment>(Entity);
			if (!OwnerFragment || OwnerFragment->OwningPlayerId != PlayerId)
			{
				continue;
			}
		}

		const FTransformFragment* Transform = EntityManager->GetFragmentDataPtr<FTransformFragment>(Entity);
		if (!Transform)
		{
			continue;
		}

		FVector2D ScreenPos;
		if (!UGameplayStatics::ProjectWorldToScreen(this, Transform->GetTransform().GetLocation(), ScreenPos))
		{
			continue;
		}

		if (ScreenPos.X >= Min.X && ScreenPos.X <= Max.X && ScreenPos.Y >= Min.Y && ScreenPos.Y <= Max.Y)
		{
			Result.Add(Entity);
		}
	}

	return Result;
}

FMassEntityHandle AMassWarSelectionPlayerController::FindNearestUnitAtScreenPos(const FVector2D& ScreenPos, float PixelRadius) const
{
	FMassEntityHandle Best;

	FMassEntityManager* EntityManager = GetEntityManager();
	UMassWarUnitRegistrySubsystem* Registry = GetWorld() ? GetWorld()->GetSubsystem<UMassWarUnitRegistrySubsystem>() : nullptr;
	if (!EntityManager || !Registry)
	{
		return Best;
	}

	float BestDistSq = FMath::Square(PixelRadius);

	for (const FMassEntityHandle& Entity : Registry->GetAllUnits())
	{
		if (!FMassWarUnitStateView::IsLiving(*EntityManager, Entity))
		{
			continue;
		}

		const FTransformFragment* Transform = EntityManager->GetFragmentDataPtr<FTransformFragment>(Entity);
		if (!Transform)
		{
			continue;
		}

		FVector2D Projected;
		if (!UGameplayStatics::ProjectWorldToScreen(this, Transform->GetTransform().GetLocation(), Projected))
		{
			continue;
		}

		const float DistSq = FVector2D::DistSquared(Projected, ScreenPos);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Entity;
		}
	}

	return Best;
}

FMassEntityManager* AMassWarSelectionPlayerController::GetEntityManager() const
{
	UMassSpawnerSubsystem* Spawner = GetWorld() ? GetWorld()->GetSubsystem<UMassSpawnerSubsystem>() : nullptr;
	return Spawner ? &Spawner->GetEntityManagerChecked() : nullptr;
}
