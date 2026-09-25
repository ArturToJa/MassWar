// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "MassEntityHandle.h"
#include "MassWarPerceptionSubsystem.generated.h"

struct FMassEntityManager;

/** A noise waiting to be heard - see UMassWarPerceptionSubsystem::ReportNoise. */
struct FMassWarNoiseEvent
{
	FVector Location = FVector::ZeroVector;
	/** Distance at which a hearer of sensitivity 1 can still hear it. */
	float Range = 0.f;
	float Loudness = 1.f;
	FMassEntityHandle Instigator;
	uint8 InstigatorTeam = 0;
	FName Tag;
};

/**
 * Entry point for things that happen to units and that they should notice: noises (hearing) and damage.
 * Sight needs no reports - UMassWarPerceptionProcessor scans for it. Gameplay code (e.g. MassWarCombat) calls
 * ReportNoise / ReportDamage; perception does the rest. Server/Standalone use only, called from processors
 * that run in sequence with UMassWarPerceptionProcessor (they are ordered before it).
 */
UCLASS()
class MASSWARPERCEPTION_API UMassWarPerceptionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * A noise at Location that units of an enemy team within Range will hear (Range scaled by their hearing
	 * sensitivity). Instigator is who made it - hearers learn that unit and where it was; noises with an invalid
	 * Instigator are ignored. Cheap: it only queues; the processor resolves it on its next run.
	 */
	void ReportNoise(const FVector& Location, float Range, FMassEntityHandle Instigator, uint8 InstigatorTeam, float Loudness = 1.f, FName Tag = NAME_None);

	/**
	 * Victim was hurt by Instigator, who is at InstigatorLocation: the victim now knows about them (with the
	 * damage sense), even if they were out of sight. Does nothing if the victim has no perception fragment.
	 */
	void ReportDamage(FMassEntityManager& EntityManager, FMassEntityHandle Victim, FMassEntityHandle Instigator, const FVector& InstigatorLocation, float Amount);

	/** Hands the queued noises to the processor (leaves the queue empty). */
	void ConsumeNoises(TArray<FMassWarNoiseEvent>& OutNoises);

	double GetNow() const;

private:
	/** Noises reported in one frame beyond this are dropped - a safety net against a runaway reporter. */
	static constexpr int32 MaxNoisesPerFrame = 4096;

	TArray<FMassWarNoiseEvent> PendingNoises;
};
