// Copyright Epic Games, Inc. All Rights Reserved.

#include "Processors/MassWarUpdateISMProcessor.h"
#include "FarAnimation/MassWarFarAnimationTypes.h"
#include "Fragments/MassWarUnitFragments.h"
#include "Weapons/MassWarWeaponSubsystem.h"
#include "Weapons/MassWarWeaponDefinition.h"
#include "MassVisualizationComponent.h"
#include "MassUpdateISMProcessor.h"
#include "MassRepresentationSubsystem.h"
#include "MassRepresentationFragments.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassEntityManager.h"
#include "MassLODFragments.h"
#include "Engine/World.h"

namespace
{
	/** A one-frame jump in position larger than this speed is a teleport/stale previous position, not walking. */
	constexpr float MaxPlausibleSpeed = 4000.f;
	/** After this long without being processed (e.g. shown as an Actor meanwhile) a unit's animation state is re-synced. */
	constexpr float ResyncGapSeconds = 0.5f;

	void StartClip(FMassWarFarAnimationFragment& Anim, const FMassWarFarAnimClip& Clip, EMassWarFarAnimRole Role, float StartTime, bool bLoop)
	{
		Anim.StartFrame = Clip.StartFrame;
		Anim.NumFrames = Clip.GetNumFrames();
		Anim.State = static_cast<uint8>(Role);
		Anim.StartTime = StartTime;
		Anim.bLoop = bLoop;
		Anim.ClipHoldType = Anim.HoldType;
	}

	/** A locomotion clip for Role, falling back to the nearest other locomotion clip the unit type has. */
	const FMassWarFarAnimClip* FindLocomotionClip(const FMassWarFarAnimationParams& Params, EMassWarFarAnimRole Role, uint32 Pick, EMassWarWeaponHoldType HoldType, EMassWarFarAnimRole& OutRole)
	{
		static const EMassWarFarAnimRole IdleChain[] = { EMassWarFarAnimRole::Idle, EMassWarFarAnimRole::Walk, EMassWarFarAnimRole::Run };
		static const EMassWarFarAnimRole WalkChain[] = { EMassWarFarAnimRole::Walk, EMassWarFarAnimRole::Run, EMassWarFarAnimRole::Idle };
		static const EMassWarFarAnimRole RunChain[] = { EMassWarFarAnimRole::Run, EMassWarFarAnimRole::Walk, EMassWarFarAnimRole::Idle };
		const EMassWarFarAnimRole* Chain = Role == EMassWarFarAnimRole::Idle ? IdleChain : (Role == EMassWarFarAnimRole::Walk ? WalkChain : RunChain);
		for (int32 Index = 0; Index < 3; ++Index)
		{
			if (const FMassWarFarAnimClip* Clip = Params.FindClipFor(Chain[Index], HoldType, Pick))
			{
				OutRole = Role; // the state stays what was asked for, so it is not re-picked every frame
				return Clip;
			}
		}
		return nullptr;
	}

	/** Advances one unit's animation state and returns the custom data its instance should show. */
	FMassWarFarAnimCustomData UpdateFarAnimation(FMassWarFarAnimationFragment& Anim, const FMassWarFarAnimationParams& Params,
		const FMassWarLifeFragment* Life, const FMassWarAttackFeedbackFragment* Attack,
		const FTransform& Transform, const FTransform& PrevTransform, FMassEntityHandle Entity, float DeltaTime, float Now,
		uint8 ActiveWeaponId, const UMassWarWeaponSubsystem* Weapons)
	{
		const bool bResync = !Anim.bInitialized || (Now - Anim.LastUpdateTime) > ResyncGapSeconds;
		Anim.LastUpdateTime = Now;

		// Speed from position changes (velocity is not replicated).
		float Sample = FVector::Dist2D(Transform.GetLocation(), PrevTransform.GetLocation()) / FMath::Max(DeltaTime, 1.e-4f);
		if (Sample > MaxPlausibleSpeed)
		{
			Sample = Anim.SmoothedSpeed;
		}
		Anim.SmoothedSpeed = bResync ? Sample : FMath::Lerp(Anim.SmoothedSpeed, Sample, FMath::Min(1.f, DeltaTime * 10.f));
		if (Anim.bMoving ? Anim.SmoothedSpeed < Params.StopMovingSpeed : Anim.SmoothedSpeed > Params.StartMovingSpeed)
		{
			Anim.bMoving = !Anim.bMoving;
		}

		// The stance of the weapon in hand (looked up only when the weapon changes).
		if (ActiveWeaponId != Anim.LastActiveWeaponId)
		{
			Anim.LastActiveWeaponId = ActiveWeaponId;
			const UMassWarWeaponDefinition* Weapon = Weapons ? Weapons->GetDefinition(ActiveWeaponId) : nullptr;
			Anim.HoldType = static_cast<uint8>(Weapon ? Weapon->HoldType : EMassWarWeaponHoldType::Unarmed);
		}

		const uint32 Pick = GetTypeHash(Entity);
		const float SecondsPerFrame = 1.f / Params.SampleRate;

		bool bHold = false;
		if (Life && Life->IsDying())
		{
			if (Anim.State != static_cast<uint8>(EMassWarFarAnimRole::Death) || !Anim.bInitialized)
			{
				if (const FMassWarFarAnimClip* Clip = Params.FindClipFor(EMassWarFarAnimRole::Death, static_cast<EMassWarWeaponHoldType>(Anim.HoldType), Pick))
				{
					// On the server TimeDying says how far into the death the unit is; on a client it stays 0 and the clip starts now.
					StartClip(Anim, *Clip, EMassWarFarAnimRole::Death, Now - Life->TimeDying, false);
				}
			}
			bHold = Anim.State == static_cast<uint8>(EMassWarFarAnimRole::Death);
		}
		else
		{
			if (Attack)
			{
				if (bResync)
				{
					Anim.LastAttackCounter = Attack->AttackCounter; // do not replay attacks that happened while not shown
				}
				else if (Attack->AttackCounter != Anim.LastAttackCounter)
				{
					Anim.LastAttackCounter = Attack->AttackCounter;
					if (const FMassWarFarAnimClip* Clip = Params.FindClipFor(EMassWarFarAnimRole::Attack, static_cast<EMassWarWeaponHoldType>(Anim.HoldType), Pick + Attack->AttackCounter))
					{
						StartClip(Anim, *Clip, EMassWarFarAnimRole::Attack, Now, false);
					}
				}
			}

			if (Anim.State == static_cast<uint8>(EMassWarFarAnimRole::Attack) && (Now - Anim.StartTime) < Anim.NumFrames * SecondsPerFrame)
			{
				bHold = true; // let the attack play out
			}
		}

		if (!bHold)
		{
			const EMassWarFarAnimRole Wanted = !Anim.bMoving ? EMassWarFarAnimRole::Idle
				: (Anim.SmoothedSpeed >= Params.RunSpeed ? EMassWarFarAnimRole::Run : EMassWarFarAnimRole::Walk);
			if (!Anim.bInitialized || Anim.State != static_cast<uint8>(Wanted) || Anim.ClipHoldType != Anim.HoldType)
			{
				EMassWarFarAnimRole ChosenRole = Wanted;
				if (const FMassWarFarAnimClip* Clip = FindLocomotionClip(Params, Wanted, Pick, static_cast<EMassWarWeaponHoldType>(Anim.HoldType), ChosenRole))
				{
					// Phase-shift each unit within the loop so a crowd does not step in lockstep.
					const float Phase = static_cast<float>(((Pick * 2654435761u) >> 8) & 0xFFFFu) / 65536.f * Clip->GetNumFrames() * SecondsPerFrame;
					StartClip(Anim, *Clip, ChosenRole, Now - Phase, true);
				}
				else
				{
					Anim.State = static_cast<uint8>(Wanted);
					Anim.ClipHoldType = Anim.HoldType;
				}
			}
		}

		Anim.bInitialized = true;

		// Translate "this clip started at StartTime" into the layer's own terms (see FMassWarFarAnimCustomData).
		FMassWarFarAnimCustomData Data;
		const float Duration = Anim.NumFrames * SecondsPerFrame;
		if (!Anim.bLoop && (Now - Anim.StartTime) >= (Anim.NumFrames - 1) * SecondsPerFrame)
		{
			// A finished one-shot clip (death): a one-frame range at play rate 0 never moves off its last frame.
			Data.StartFrame = Data.EndFrame = static_cast<float>(Anim.StartFrame + Anim.NumFrames - 1);
			Data.Playrate = 0.f;
		}
		else
		{
			Data.StartFrame = static_cast<float>(Anim.StartFrame);
			Data.EndFrame = static_cast<float>(Anim.StartFrame + Anim.NumFrames - 1);
			// The layer's clock is (Time + TimeOffset), clamped to be non-negative: pick the offset that puts it on a
			// whole number of clip lengths at StartTime, so the clip begins at its first frame then.
			const float Phase = Anim.StartTime - Duration * FMath::FloorToFloat(Anim.StartTime / Duration);
			Data.TimeOffset = Phase > 0.f ? Duration - Phase : 0.f;
		}
		return Data;
	}
}

UMassWarUpdateISMProcessor::UMassWarUpdateISMProcessor()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	// Same as the engine processor it replaces.
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Client | EProcessorExecutionFlags::Standalone);
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Representation);
	bRequiresGameThreadExecution = true;
}

void UMassWarUpdateISMProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	// The engine processor's requirements (this replaces it)...
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassRepresentationFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddChunkRequirement<FMassVisualizationChunkFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.SetChunkFilter(&FMassVisualizationChunkFragment::AreAnyEntitiesVisibleInChunk);
	EntityQuery.AddSharedRequirement<FMassRepresentationSubsystemSharedFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FMassStaticRepresentationTag>(EMassFragmentPresence::None);

	// ...plus the far animation data (absent on unit types without the trait).

	EntityQuery.AddRequirement<FMassWarFarAnimationFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::Optional);
	EntityQuery.AddRequirement<FMassWarLifeFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);
	EntityQuery.AddRequirement<FMassWarAttackFeedbackFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);
	EntityQuery.AddConstSharedRequirement<FMassWarFarAnimationParams>(EMassFragmentPresence::Optional);
	EntityQuery.AddRequirement<FMassWarLoadoutFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);
}

void UMassWarUpdateISMProcessor::UpdateInstancedWeapon(FMassEntityHandle Entity, const uint8 ActiveWeaponId, const FMassWarFarAnimationParams& Params, const FMassWarFarAnimCustomData& Anim, const float Now,
	const FTransform& Transform, const FTransform& PrevTransform, const float LODSignificance, const float PrevLODSignificance, const FStaticMeshInstanceVisualizationDesc& BodyDesc,
	FMassInstancedStaticMeshInfoArrayView& Infos, const UMassWarWeaponSubsystem* Weapons)
{
	if (ActiveWeaponId == 0 || !Weapons)
	{
		return;
	}
	if (WeaponVisuals.Num() <= ActiveWeaponId)
	{
		WeaponVisuals.SetNum(256);
	}
	FWeaponVisual& Visual = WeaponVisuals[ActiveWeaponId];

	if (Visual.State == FWeaponVisual::EState::Unresolved)
	{
		const UMassWarWeaponDefinition* Weapon = Weapons->GetDefinition(ActiveWeaponId);
		if (Weapon && Weapon->bShowOnInstancedUnits && !Weapon->Mesh.IsNull())
		{
			// The mesh set cannot be built here (the engine's mesh list is being read by this very loop): ask for it. It is
			// built right after the loop, and the weapon appears a frame or two later.
			Visual.State = FWeaponVisual::EState::Pending;
			Visual.AttachOffset = Weapon->AttachOffset;
			Visual.MinLODSignificance = static_cast<float>(Weapon->InstancedLODSignificanceRange.X);
			Visual.MaxLODSignificance = static_cast<float>(Weapon->InstancedLODSignificanceRange.Y);
			PendingWeapons.Add(ActiveWeaponId);
		}
		else
		{
			Visual.State = FWeaponVisual::EState::None;
		}
		return;
	}

	// Outside its range the weapon is simply not pushed (and so not drawn); a mesh set that was just added is usable a little later still.
	if (Visual.State != FWeaponVisual::EState::Ready || LODSignificance < Visual.MinLODSignificance || LODSignificance >= Visual.MaxLODSignificance
		|| !Visual.Handle.IsValid() || !Infos.IsValidIndex(Visual.Handle.ToIndex()) || !Infos[Visual.Handle.ToIndex()].IsValid())
	{
		return;
	}

	// The frame of the baked animation the unit is showing, as the animation layer works it out.
	const float NumFrames = Anim.EndFrame - Anim.StartFrame + 1.f;
	const float Frame = Anim.Playrate > 0.f ? Anim.StartFrame + FMath::Fmod((Now + Anim.TimeOffset) * Anim.Playrate * Params.SampleRate, NumFrames) : Anim.StartFrame;
	FTransform Hand;
	if (!Params.SampleHand(Frame, FMath::RoundToInt(Anim.StartFrame), FMath::RoundToInt(Anim.EndFrame), Hand))
	{
		return;
	}

	// Where the baked body sits relative to the unit: the mesh's own placement inside the instance, then the set's offset.
	// (The body mesh that overlaps the weapon's range is used, as the unit type may use different meshes at different distances.)
	FTransform BodyPlacement = FTransform::Identity;
	if (!BodyDesc.Meshes.IsEmpty())
	{
		const FMassStaticMeshInstanceVisualizationMeshDesc* Mesh = &BodyDesc.Meshes[0];
		for (const FMassStaticMeshInstanceVisualizationMeshDesc& Candidate : BodyDesc.Meshes)
		{
			if (Candidate.MinLODSignificance <= LODSignificance && LODSignificance < Candidate.MaxLODSignificance)
			{
				Mesh = &Candidate;
				break;
			}
		}
		BodyPlacement = Mesh->LocalTransform;
	}
	if (BodyDesc.bUseTransformOffset)
	{
		BodyPlacement = BodyPlacement * BodyDesc.TransformOffset;
	}

	const FTransform InUnit = Visual.AttachOffset * Hand * BodyPlacement;
	Infos[Visual.Handle.ToIndex()].AddBatchedTransform(Entity, InUnit * Transform, InUnit * PrevTransform, LODSignificance, PrevLODSignificance);
}

void UMassWarUpdateISMProcessor::CreatePendingWeaponDescs(const UMassWarWeaponSubsystem* Weapons)
{
	UMassRepresentationSubsystem* RepresentationSubsystem = PendingSubsystem.Get();
	if (PendingWeapons.IsEmpty() || !RepresentationSubsystem)
	{
		PendingWeapons.Reset();
		return;
	}

	for (const uint8 WeaponId : PendingWeapons)
	{
		FWeaponVisual& Visual = WeaponVisuals[WeaponId];
		const UMassWarWeaponDefinition* Weapon = Weapons ? Weapons->GetDefinition(WeaponId) : nullptr;
		UStaticMesh* WeaponMesh = Weapon ? Weapon->LoadMesh() : nullptr;
		if (!WeaponMesh)
		{
			UE_LOG(LogTemp, Warning, TEXT("MassWar instanced weapons: weapon %s has no loadable mesh - instanced units will not show it."), *GetNameSafe(Weapon));
			Visual.State = FWeaponVisual::EState::None;
			continue;
		}

		FStaticMeshInstanceVisualizationDesc Desc;
		FMassStaticMeshInstanceVisualizationMeshDesc& MeshDesc = Desc.Meshes.AddDefaulted_GetRef();
		MeshDesc.Mesh = WeaponMesh;
		MeshDesc.bCastShadows = Weapon->bInstancedCastShadows;
		MeshDesc.MinLODSignificance = Visual.MinLODSignificance;
		MeshDesc.MaxLODSignificance = Visual.MaxLODSignificance;
		Visual.Handle = RepresentationSubsystem->FindOrAddStaticMeshDesc(Desc);
		Visual.State = FWeaponVisual::EState::Ready;
	}
	PendingWeapons.Reset();
}

void UMassWarUpdateISMProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const UWorld* World = EntityManager.GetWorld();
	const float Now = World ? static_cast<float>(World->GetTimeSeconds()) : 0.f;
	const UMassWarWeaponSubsystem* Weapons = World ? World->GetSubsystem<UMassWarWeaponSubsystem>() : nullptr;

	EntityQuery.ForEachEntityChunk(Context, [this, Now, Weapons](FMassExecutionContext& Context)
	{
		UMassRepresentationSubsystem* RepresentationSubsystem = Context.GetSharedFragment<FMassRepresentationSubsystemSharedFragment>().RepresentationSubsystem;
		check(RepresentationSubsystem);
		if (PendingSubsystem.Get() != RepresentationSubsystem)
		{
			// Mesh sets belong to one representation subsystem; a different one (new world) starts fresh.
			WeaponVisuals.Reset();
			PendingWeapons.Reset();
			PendingSubsystem = RepresentationSubsystem;
		}
		FMassInstancedStaticMeshInfoArrayView ISMInfo = RepresentationSubsystem->GetMutableInstancedStaticMeshInfos();

		const TConstArrayView<FTransformFragment> TransformList = Context.GetFragmentView<FTransformFragment>();
		const TArrayView<FMassRepresentationFragment> RepresentationList = Context.GetMutableFragmentView<FMassRepresentationFragment>();
		const TConstArrayView<FMassRepresentationLODFragment> RepresentationLODList = Context.GetFragmentView<FMassRepresentationLODFragment>();

		// Present only for unit types with the Far Animation trait (a whole chunk has all of these or none).
		const TArrayView<FMassWarFarAnimationFragment> AnimList = Context.GetMutableFragmentView<FMassWarFarAnimationFragment>();
		const FMassWarFarAnimationParams* AnimParams = Context.GetConstSharedFragmentPtr<FMassWarFarAnimationParams>();
		const TConstArrayView<FMassWarLifeFragment> LifeList = Context.GetFragmentView<FMassWarLifeFragment>();
		const TConstArrayView<FMassWarAttackFeedbackFragment> AttackList = Context.GetFragmentView<FMassWarAttackFeedbackFragment>();
		const TConstArrayView<FMassWarLoadoutFragment> LoadoutList = Context.GetFragmentView<FMassWarLoadoutFragment>();
		const bool bAnimate = AnimParams && !AnimList.IsEmpty();
		const float DeltaTime = Context.GetDeltaTimeSeconds();

		for (FMassExecutionContext::FEntityIterator EntityIt = Context.CreateEntityIterator(); EntityIt; ++EntityIt)
		{
			const FTransformFragment& TransformFragment = TransformList[EntityIt];
			const FMassRepresentationLODFragment& RepresentationLOD = RepresentationLODList[EntityIt];
			FMassRepresentationFragment& Representation = RepresentationList[EntityIt];

			if (Representation.CurrentRepresentation == EMassRepresentationType::StaticMeshInstance)
			{
				const uint8 ActiveWeaponId = LoadoutList.IsEmpty() ? 0 : LoadoutList[EntityIt].GetActiveWeaponId();

				const int32 ISMInfoIndex = Representation.StaticMeshDescHandle.ToIndex();
				if (ensureMsgf(ISMInfo.IsValidIndex(ISMInfoIndex), TEXT("Invalid handle index %u for ISMInfosView"), ISMInfoIndex))
				{
					UMassUpdateISMProcessor::UpdateISMTransform(Context.GetEntity(EntityIt), ISMInfo[ISMInfoIndex], TransformFragment.GetTransform(), Representation.PrevTransform, RepresentationLOD.LODSignificance, Representation.PrevLODSignificance);

					if (bAnimate)
					{
						// Must follow the transform for every instance, in the same order (see class comment).
						const FMassWarFarAnimCustomData Data = UpdateFarAnimation(AnimList[EntityIt], *AnimParams,
							LifeList.IsEmpty() ? nullptr : &LifeList[EntityIt], AttackList.IsEmpty() ? nullptr : &AttackList[EntityIt],
							TransformFragment.GetTransform(), Representation.PrevTransform, Context.GetEntity(EntityIt), DeltaTime, Now,
							ActiveWeaponId, Weapons);
						ISMInfo[ISMInfoIndex].AddBatchedCustomData(Data, RepresentationLOD.LODSignificance, Representation.PrevLODSignificance);

							// The unit's weapon, in its hand (not while dying: the body falls over, the weapon would not).
							if (ActiveWeaponId != 0 && !AnimParams->HandTrack.IsEmpty() && (LifeList.IsEmpty() || !LifeList[EntityIt].IsDying()))
							{
								UpdateInstancedWeapon(Context.GetEntity(EntityIt), ActiveWeaponId, *AnimParams, Data, Now, TransformFragment.GetTransform(), Representation.PrevTransform,
									RepresentationLOD.LODSignificance, Representation.PrevLODSignificance, ISMInfo[ISMInfoIndex].GetDesc(), ISMInfo, Weapons);
							}
					}
				}
			}
			Representation.PrevTransform = TransformFragment.GetTransform();
			Representation.PrevLODSignificance = RepresentationLOD.LODSignificance;
		}
	});

	// Every view of the engine's mesh list is released now: new mesh sets requested during the loop can be built.
	CreatePendingWeaponDescs(Weapons);
}
