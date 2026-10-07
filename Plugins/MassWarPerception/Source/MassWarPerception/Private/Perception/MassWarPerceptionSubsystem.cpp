// Copyright Epic Games, Inc. All Rights Reserved.

#include "Perception/MassWarPerceptionSubsystem.h"
#include "Fragments/MassWarPerceptionFragments.h"
#include "MassEntityManager.h"
#include "Engine/World.h"

double UMassWarPerceptionSubsystem::GetNow() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

void UMassWarPerceptionSubsystem::ReportNoise(const FVector& Location, float Range, FMassEntityHandle Instigator, uint8 InstigatorTeam, float Loudness, FName Tag)
{
	if (Range <= 0.f || !Instigator.IsValid() || PendingNoises.Num() >= MaxNoisesPerFrame)
	{
		return;
	}
	FMassWarNoiseEvent& Noise = PendingNoises.AddDefaulted_GetRef();
	Noise.Location = Location;
	Noise.Range = Range;
	Noise.Loudness = Loudness;
	Noise.Instigator = Instigator;
	Noise.InstigatorTeam = InstigatorTeam;
	Noise.Tag = Tag;
}

void UMassWarPerceptionSubsystem::ReportDamage(FMassEntityManager& EntityManager, FMassEntityHandle Victim, FMassEntityHandle Instigator, const FVector& InstigatorLocation, float Amount)
{
	if (!Instigator.IsValid() || !EntityManager.IsEntityValid(Victim))
	{
		return;
	}
	FMassWarPerceptionFragment* Perception = EntityManager.GetFragmentDataPtr<FMassWarPerceptionFragment>(Victim);
	if (!Perception)
	{
		return;
	}
	if (FMassWarPerceivedEntry* Entry = Perception->FindOrAdd(Instigator))
	{
		Entry->LastKnownLocation = FVector3f(InstigatorLocation);
		Entry->LastStimulusTime = GetNow();
		Entry->LastDamageTime = Entry->LastStimulusTime;
		Entry->Strength = Amount;
		Entry->LastSense = EMassWarSense::Damage;
	}
}

void UMassWarPerceptionSubsystem::ConsumeNoises(TArray<FMassWarNoiseEvent>& OutNoises)
{
	OutNoises.Reset();
	Swap(OutNoises, PendingNoises);
}
