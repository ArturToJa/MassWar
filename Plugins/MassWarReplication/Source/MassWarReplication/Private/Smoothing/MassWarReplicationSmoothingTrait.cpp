// Copyright Epic Games, Inc. All Rights Reserved.

#include "Smoothing/MassWarReplicationSmoothingTrait.h"
#include "MassEntityTemplateRegistry.h"
#include "Smoothing/MassWarClientInterpolationFragment.h"

void UMassWarReplicationSmoothingTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	// Purely a client-side visual concern. Must NOT be added on Server/ListenServer/Standalone: a listen
	// server's own local view satisfies EProcessorExecutionFlags::Client too (it's simultaneously server
	// and its own client), so UMassWarClientInterpolationProcessor would otherwise also run there and
	// drag the server's authoritative entities toward this fragment's unset (0,0,0) default target.
	if (!World.IsNetMode(NM_Client))
	{
		return;
	}

	FMassWarClientInterpolationFragment& Fragment = BuildContext.AddFragment_GetRef<FMassWarClientInterpolationFragment>();
	Fragment.InterpSpeed = InterpSpeed;
}
