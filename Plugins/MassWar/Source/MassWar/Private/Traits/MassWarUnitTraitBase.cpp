// Copyright Epic Games, Inc. All Rights Reserved.

#include "Traits/MassWarUnitTraitBase.h"
#include "MassEntityTemplateRegistry.h"
#include "MassCommonFragments.h"
#include "MassActorSubsystem.h"
#include "MassMovementFragments.h"
#include "Fragments/MassWarUnitFragments.h"
#include "Stats/MassWarStatSubsystem.h"

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
		BuildContext.AddFragment<FMassWarAvoidanceFragment>();
	}
	BuildContext.AddFragment<FMassWarFormationMemberFragment>();
	BuildContext.AddFragment<FMassWarStatSetsFragment>(); // run-time stat sets (upgrades, buffs); replicated
	BuildContext.AddFragment<FMassWarNetIdFragment>();
	BuildContext.AddFragment<FMassWarOwnerFragment>();

	// The numbers other systems read are stats: this trait supplies the unit type's base values (a MassWar Stats trait,
	// weapons and upgrades can change them; the order of traits does not matter).
	UMassWarStatSubsystem::ContributeBase(BuildContext, World, EMassWarStat::MoveSpeed, MoveSpeed);
	UMassWarStatSubsystem::ContributeBase(BuildContext, World, EMassWarStat::TurnRate, TurnRate);
	UMassWarStatSubsystem::ContributeBase(BuildContext, World, EMassWarStat::AcceptanceRadius, AcceptanceRadius);
	UMassWarStatSubsystem::ContributeBase(BuildContext, World, EMassWarStat::AttackStopDistance, AttackStopDistance);
	UMassWarStatSubsystem::ContributeBase(BuildContext, World, EMassWarStat::AvoidanceRadius, AvoidanceRadius);
	UMassWarStatSubsystem::ContributeBase(BuildContext, World, EMassWarStat::AvoidanceStrength, AvoidanceStrength);
	UMassWarStatSubsystem::ContributeBase(BuildContext, World, EMassWarStat::DeathLingerTime, DeathLingerTime);
}
