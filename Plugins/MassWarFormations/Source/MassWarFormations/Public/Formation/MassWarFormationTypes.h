// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityHandle.h"
#include "Fragments/MassWarPerceptionFragments.h"
#include "MassWarFormationTypes.generated.h"

/** The layout a formation arranges its members in around a destination. Members walk to their own slot on
 *  their own, so even a "Line" arrives as a loose crowd until everyone has reached their place. */
UENUM(BlueprintType)
enum class EMassWarFormationShape : uint8
{
	/** Members spread evenly over a disc - no ranks, no facing. */
	Blob,
	/** One rank, side by side, perpendicular to the direction of travel. */
	Line,
	/** Several ranks (roughly twice as wide as deep), facing the direction of travel. */
	Grid
};

USTRUCT(BlueprintType)
struct MASSWARFORMATIONS_API FMassWarFormationSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Formation")
	EMassWarFormationShape Shape = EMassWarFormationShape::Blob;

	/** Distance (uu) between neighbouring slots. Keep it above twice the units' avoidance radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Formation", meta = (ClampMin = "10.0"))
	float Spacing = 130.f;

	/** While idle, the formation attacks the nearest enemy formation any of its members can currently see (the
	 *  formation's pooled knowledge), if it is within AutoEngageRadius. Off = it only fights when ordered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Formation")
	bool bAutoEngage = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Formation", meta = (EditCondition = "bAutoEngage", ClampMin = "0.0"))
	float AutoEngageRadius = 3000.f;

	/** While idle and nothing is in sight, walk to where a member just heard a noise or was hit from. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Formation")
	bool bInvestigate = true;

	/** How long (seconds) the formation remembers what its members perceived after they stop perceiving it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MassWar|Formation", meta = (ClampMin = "0.0"))
	float KnowledgeMemory = 8.f;
};

enum class EMassWarFormationOrderType : uint8
{
	Idle,
	Move,
	/** Attack another formation: the formation assigns each member a target inside it. */
	Attack
};

/** Server-side formation record (see UMassWarFormationSubsystem). Deliberately plain data, not an entity:
 *  there are tens to hundreds of formations, not tens of thousands of units. */
struct FMassWarFormation
{
	uint32 Id = 0;
	uint32 OwnerPlayerId = 0;
	uint8 TeamId = 0;
	FMassWarFormationSettings Settings;

	TArray<FMassEntityHandle> Members;

	/** Centre of the living members, refreshed every subsystem tick. */
	FVector Anchor = FVector::ZeroVector;

	EMassWarFormationOrderType OrderType = EMassWarFormationOrderType::Idle;
	uint32 TargetFormationId = 0;

	/** Everything any member perceives, pooled (freshest stimulus per unit, in sight if any member sees it):
	 *  what one member sees, hears or is hurt by, the whole formation knows. Capped at MaxKnowledge entries. */
	TArray<FMassWarPerceivedEntry> Knowledge;
	static constexpr int32 MaxKnowledge = 32;

	/** Stimulus time of the last noise/hit this formation walked over to check out, so it investigates each once. */
	double LastInvestigatedTime = -1.0e9;
};
