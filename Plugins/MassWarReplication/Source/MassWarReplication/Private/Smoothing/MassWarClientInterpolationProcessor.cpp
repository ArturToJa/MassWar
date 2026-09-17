// Copyright Epic Games, Inc. All Rights Reserved.

#include "Smoothing/MassWarClientInterpolationProcessor.h"
#include "Smoothing/MassWarClientInterpolationFragment.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"

UMassWarClientInterpolationProcessor::UMassWarClientInterpolationProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)EProcessorExecutionFlags::Client;
}

void UMassWarClientInterpolationProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassWarClientInterpolationFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
}

void UMassWarClientInterpolationProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const float DeltaTime = Context.GetDeltaTimeSeconds();

	EntityQuery.ForEachEntityChunk(Context, [DeltaTime](FMassExecutionContext& Context)
	{
		const TConstArrayView<FMassWarClientInterpolationFragment> InterpList = Context.GetFragmentView<FMassWarClientInterpolationFragment>();
		const TArrayView<FTransformFragment> TransformList = Context.GetMutableFragmentView<FTransformFragment>();

		for (FMassExecutionContext::FEntityIterator It = Context.CreateEntityIterator(); It; ++It)
		{
			const FMassWarClientInterpolationFragment& Interp = InterpList[It];
			FTransform& Transform = TransformList[It].GetMutableTransform();

			const float Alpha = FMath::Clamp(DeltaTime * Interp.InterpSpeed, 0.f, 1.f);
			Transform.SetLocation(FMath::VInterpTo(Transform.GetLocation(), Interp.TargetPosition, DeltaTime, Interp.InterpSpeed));
			Transform.SetRotation(FQuat::Slerp(Transform.GetRotation(), Interp.TargetRotation, Alpha).GetNormalized());
		}
	});
}
