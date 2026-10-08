// Copyright Epic Games, Inc. All Rights Reserved.

#include "Traits/MassWarFarAnimationTrait.h"
#include "MassEntityTemplateRegistry.h"
#include "MassEntityUtils.h"
#include "MassCommonFragments.h"
#include "Engine/World.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AttributesRuntime.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "Misc/MemStack.h"
#include "Engine/SkeletalMeshSocket.h"

namespace
{
	/** The hand bone in mesh space for every baked frame, the way the bake sampled it: frame i of a clip is the animation at
	 *  i / SampleRate, evaluated with the skeletal mesh's own bones (so retargeting matches). */
	void BuildHandTrack(const TArray<FMassWarFarAnimClip>& Clips, const USkeletalMesh& Mesh, FName HandBone, float SampleRate, TArray<FTransform>& OutTrack)
	{
		// The hand is a bone, or a socket on the mesh (a bone plus a fixed offset), as for the actor's weapon socket.
		FTransform SocketOffset = FTransform::Identity;
		int32 HandIndex = Mesh.GetRefSkeleton().FindBoneIndex(HandBone);
		if (HandIndex == INDEX_NONE)
		{
			if (const USkeletalMeshSocket* Socket = Mesh.FindSocket(HandBone))
			{
				HandIndex = Mesh.GetRefSkeleton().FindBoneIndex(Socket->BoneName);
				SocketOffset = Socket->GetSocketLocalTransform();
			}
		}
		if (HandIndex == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("MassWar Far Animation: %s is neither a bone nor a socket of %s - weapons will not follow a hand."), *HandBone.ToString(), *Mesh.GetName());
			return;
		}

		TArray<FBoneIndexType> RequiredBones;
		for (int32 Bone = 0; Bone < Mesh.GetRefSkeleton().GetNum(); ++Bone)
		{
			RequiredBones.Add(static_cast<FBoneIndexType>(Bone));
		}
		FBoneContainer BoneContainer(RequiredBones, UE::Anim::FCurveFilterSettings(), const_cast<USkeletalMesh&>(Mesh));
		const FCompactPoseBoneIndex CompactHand = BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(HandIndex));

		int32 NumFrames = 0;
		for (const FMassWarFarAnimClip& Clip : Clips)
		{
			NumFrames = Clip.Animation ? FMath::Max(NumFrames, Clip.EndFrame + 1) : NumFrames;
		}
		OutTrack.Init(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector), NumFrames);

		for (const FMassWarFarAnimClip& Clip : Clips)
		{
			if (!Clip.Animation)
			{
				continue;
			}
			for (int32 Frame = Clip.StartFrame; Frame <= Clip.EndFrame; ++Frame)
			{
				// The pose containers allocate from the per-thread memory stack, which must be marked.
				FMemMark MemMark(FMemStack::Get());
				FCompactPose Pose;
				Pose.SetBoneContainer(&BoneContainer);
				Pose.ResetToRefPose();
				FBlendedCurve Curve;
				Curve.InitFrom(BoneContainer);
				UE::Anim::FStackAttributeContainer Attributes;
				FAnimationPoseData PoseData(Pose, Curve, Attributes);

				const double Time = FMath::Min((Frame - Clip.StartFrame) / static_cast<double>(SampleRate), static_cast<double>(Clip.Animation->GetPlayLength()));
				Clip.Animation->GetAnimationPose(PoseData, FAnimExtractContext(Time));

				FCSPose<FCompactPose> ComponentPose;
				ComponentPose.InitPose(Pose);
				FTransform Hand = SocketOffset * ComponentPose.GetComponentSpaceTransform(CompactHand);
				Hand.SetScale3D(FVector::OneVector);
				OutTrack[Frame] = Hand;
			}
		}
	}
}

void UMassWarFarAnimationTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.RequireFragment<FTransformFragment>();
	BuildContext.AddFragment<FMassWarFarAnimationFragment>();

	FMassWarFarAnimationParams Params;
	Params.Clips = Clips;
	Params.SampleRate = FMath::Max(SampleRate, 1.f);
	Params.StartMovingSpeed = StartMovingSpeed;
	Params.StopMovingSpeed = FMath::Min(StopMovingSpeed, StartMovingSpeed);
	Params.RunSpeed = RunSpeed;

	if (SkeletalMesh)
	{
		BuildHandTrack(Clips, *SkeletalMesh, HandBoneName, Params.SampleRate, Params.HandTrack);
	}
// The animations were only needed for the track above.
	for (FMassWarFarAnimClip& Clip : Params.Clips)
	{
		Clip.Animation = nullptr;
	}

	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);
	BuildContext.AddConstSharedFragment(EntityManager.GetOrCreateConstSharedFragment(Params));
}
