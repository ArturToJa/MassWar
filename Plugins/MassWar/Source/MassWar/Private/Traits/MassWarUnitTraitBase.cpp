// Copyright Epic Games, Inc. All Rights Reserved.

#include "Traits/MassWarUnitTraitBase.h"
#include "MassEntityTemplateRegistry.h"
#include "MassCommonFragments.h"
#include "MassActorSubsystem.h"
#include "MassMovementFragments.h"
#include "Fragments/MassWarUnitFragments.h"

void UMassWarUnitTraitBase::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	// Transform/Velocity are expected to be added by whichever movement trait is also on the config; we
	// only require them here rather than adding them ourselves, to avoid "added by two traits" conflicts.
	BuildContext.RequireFragment<FTransformFragment>();
	BuildContext.RequireFragment<FMassVelocityFragment>();

	// Universal "slot" fragment tracking this entity's representation actor, if any - required by
	// every Mass visualization trait regardless of whether an actor ever actually gets spawned.
	BuildContext.AddFragment<FMassActorFragment>();

	FMassWarTeamFragment& Team = BuildContext.AddFragment_GetRef<FMassWarTeamFragment>();
	Team.TeamId = DefaultTeamId;

	BuildContext.AddFragment<FMassWarOrderFragment>();

	FMassWarLifeFragment& Life = BuildContext.AddFragment_GetRef<FMassWarLifeFragment>();
	Life.LingerTime = DeathLingerTime;

	BuildContext.AddFragment<FMassWarAttackFeedbackFragment>();

	if (bUseNavMesh)
	{
		FMassWarNavPathFragment& Nav = BuildContext.AddFragment_GetRef<FMassWarNavPathFragment>();
		Nav.RepathInterval = NavRepathInterval;
		Nav.RepathDistance = NavRepathDistance;
	}
	if (bAvoidOtherUnits)
	{
		FMassWarAvoidanceFragment& Avoidance = BuildContext.AddFragment_GetRef<FMassWarAvoidanceFragment>();
		Avoidance.Radius = AvoidanceRadius;
		Avoidance.Strength = AvoidanceStrength;
	}
	BuildContext.AddFragment<FMassWarFormationMemberFragment>();
	BuildContext.AddFragment<FMassWarNetIdFragment>();
	BuildContext.AddFragment<FMassWarOwnerFragment>();

	FMassWarMovementParamsFragment& MovementParams = BuildContext.AddFragment_GetRef<FMassWarMovementParamsFragment>();
	MovementParams.MoveSpeed = MoveSpeed;
	MovementParams.AcceptanceRadius = AcceptanceRadius;
	MovementParams.AttackStopDistance = AttackStopDistance;
	MovementParams.TurnRate = TurnRate;
}
